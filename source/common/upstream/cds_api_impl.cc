#include "common/upstream/cds_api_impl.h"

#include <string>

#include "envoy/api/v2/cds.pb.validate.h"
#include "envoy/api/v2/cluster/outlier_detection.pb.validate.h"
#include "envoy/stats/scope.h"

#include "common/common/cleanup.h"
#include "common/common/utility.h"
#include "common/config/resources.h"
#include "common/config/subscription_factory.h"
#include "common/config/utility.h"
#include "common/protobuf/utility.h"

namespace Envoy {
namespace Upstream {

CdsApiPtr CdsApiImpl::create(const envoy::api::v2::core::ConfigSource& cds_config,
                             ClusterManager& cm, Event::Dispatcher& dispatcher,
                             Runtime::RandomGenerator& random,
                             const LocalInfo::LocalInfo& local_info, Stats::Scope& scope,
                             ProtobufMessage::ValidationVisitor& validation_visitor,
                             Api::Api& api) {
  return CdsApiPtr{new CdsApiImpl(cds_config, cm, dispatcher, random, local_info, scope,
                                  validation_visitor, api)};
}

CdsApiImpl::CdsApiImpl(const envoy::api::v2::core::ConfigSource& cds_config, ClusterManager& cm,
                       Event::Dispatcher& dispatcher, Runtime::RandomGenerator& random,
                       const LocalInfo::LocalInfo& local_info, Stats::Scope& scope,
                       ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api)
    : cm_(cm), scope_(scope.createScope("cluster_manager.cds.")),
      validation_visitor_(validation_visitor) {
  Config::Utility::checkLocalInfo("cds", local_info);

  subscription_ = Config::SubscriptionFactory::subscriptionFromConfigSource(
      cds_config, local_info, dispatcher, cm, random, *scope_,
      Grpc::Common::typeUrl(envoy::api::v2::Cluster().GetDescriptor()->full_name()),
      validation_visitor, api, *this);
}

void CdsApiImpl::onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                                const std::string& version_info) {
  ClusterManager::ClusterInfoMap clusters_to_remove = cm_.clusters();
  std::vector<envoy::api::v2::Cluster> clusters;
  for (const auto& cluster_blob : resources) {
    clusters.push_back(
        MessageUtil::anyConvert<envoy::api::v2::Cluster>(cluster_blob, validation_visitor_));
    clusters_to_remove.erase(clusters.back().name());
  }
  Protobuf::RepeatedPtrField<std::string> to_remove_repeated;
  for (const auto& cluster : clusters_to_remove) {
    *to_remove_repeated.Add() = cluster.first;
  }
  Protobuf::RepeatedPtrField<envoy::api::v2::Resource> to_add_repeated;
  for (const auto& cluster : clusters) {
    envoy::api::v2::Resource* to_add = to_add_repeated.Add();
    to_add->set_name(cluster.name());
    to_add->set_version(version_info);
    to_add->mutable_resource()->PackFrom(cluster);
  }
  onConfigUpdate(to_add_repeated, to_remove_repeated, version_info);
}

void CdsApiImpl::onConfigUpdate(
    const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>& added_resources,
    const Protobuf::RepeatedPtrField<std::string>& removed_resources,
    const std::string& system_version_info) {
  cm_.adsMux().pause(Config::TypeUrl::get().ClusterLoadAssignment);
  Cleanup eds_resume([this] { cm_.adsMux().resume(Config::TypeUrl::get().ClusterLoadAssignment); });

  std::vector<std::string> exception_msgs;
  std::unordered_set<std::string> cluster_names;
  bool any_applied = false;
  for (const auto& resource : added_resources) {
    envoy::api::v2::Cluster cluster;
    try {
      cluster = MessageUtil::anyConvert<envoy::api::v2::Cluster>(resource.resource(),
                                                                 validation_visitor_);
      MessageUtil::validate(cluster);
      if (!cluster_names.insert(cluster.name()).second) {
        // NOTE: at this point, the first of these duplicates has already been successfully applied.
        throw EnvoyException(fmt::format("duplicate cluster {} found", cluster.name()));
      }
      if (cm_.addOrUpdateCluster(
              cluster, resource.version(),
              [this](const std::string&, ClusterManager::ClusterWarmingState state) {
                // Following if/else block implements a control flow mechanism that can be used
                // by an ADS implementation to properly sequence CDS and RDS update. It is not
                // enforcing on ADS. ADS can use it to detect when a previously sent cluster becomes
                // warm before sending routes that depend on it. This can improve incidence of HTTP
                // 503 responses from Envoy when a route is used before it's supporting cluster is
                // ready.
                //
                // We achieve that by leaving CDS in the paused state as long as there is at least
                // one cluster in the warming state. This prevents CDS ACK from being sent to ADS.
                // Once cluster is warmed up, CDS is resumed, and ACK is sent to ADS, providing a
                // signal to ADS to proceed with RDS updates.
                //
                // Major concern with this approach is CDS being left in the paused state forever.
                // As long as ClusterManager::removeCluster() is not called on a warming cluster
                // this is not an issue. CdsApiImpl takes care of doing this properly, and there
                // is no other component removing clusters from the ClusterManagerImpl. If this
                // ever changes, we would need to correct the following logic.
                if (state == ClusterManager::ClusterWarmingState::Starting &&
                    cm_.warmingClusterCount() == 1) {
                  cm_.adsMux().pause(Config::TypeUrl::get().Cluster);
                } else if (state == ClusterManager::ClusterWarmingState::Finished &&
                           cm_.warmingClusterCount() == 0) {
                  cm_.adsMux().resume(Config::TypeUrl::get().Cluster);
                }
              })) {
        any_applied = true;
        ENVOY_LOG(debug, "cds: add/update cluster '{}'", cluster.name());
      }
    } catch (const EnvoyException& e) {
      exception_msgs.push_back(fmt::format("{}: {}", cluster.name(), e.what()));
    }
  }
  for (auto resource_name : removed_resources) {
    if (cm_.removeCluster(resource_name)) {
      any_applied = true;
      ENVOY_LOG(debug, "cds: remove cluster '{}'", resource_name);
    }
  }

  if (any_applied) {
    system_version_info_ = system_version_info;
  }
  runInitializeCallbackIfAny();
  if (!exception_msgs.empty()) {
    throw EnvoyException(
        fmt::format("Error adding/updating cluster(s) {}", StringUtil::join(exception_msgs, ", ")));
  }
}

void CdsApiImpl::onConfigUpdateFailed(const EnvoyException*) {
  // We need to allow server startup to continue, even if we have a bad
  // config.
  runInitializeCallbackIfAny();
}

void CdsApiImpl::runInitializeCallbackIfAny() {
  if (initialize_callback_) {
    initialize_callback_();
    initialize_callback_ = nullptr;
  }
}

} // namespace Upstream
} // namespace Envoy
