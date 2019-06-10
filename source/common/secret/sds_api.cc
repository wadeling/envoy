#include "common/secret/sds_api.h"

#include <unordered_map>

#include "envoy/api/v2/auth/cert.pb.validate.h"

#include "common/config/resources.h"
#include "common/config/subscription_factory.h"
#include "common/config/utility.h"
#include "common/protobuf/utility.h"

namespace Envoy {
namespace Secret {

SdsApi::SdsApi(const LocalInfo::LocalInfo& local_info, Event::Dispatcher& dispatcher,
               Runtime::RandomGenerator& random, Stats::Store& stats,
               Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
               const envoy::api::v2::core::ConfigSource& sds_config,
               const std::string& sds_config_name, std::function<void()> destructor_cb,
               ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api)
    : init_target_(fmt::format("SdsApi {}", sds_config_name), [this] { initialize(); }),
      local_info_(local_info), dispatcher_(dispatcher), random_(random), stats_(stats),
      cluster_manager_(cluster_manager), sds_config_(sds_config), sds_config_name_(sds_config_name),
      secret_hash_(0), clean_up_(destructor_cb), validation_visitor_(validation_visitor),
      api_(api) {
  Config::Utility::checkLocalInfo("sds", local_info_);
  // TODO(JimmyCYJ): Implement chained_init_manager, so that multiple init_manager
  // can be chained together to behave as one init_manager. In that way, we let
  // two listeners which share same SdsApi to register at separate init managers, and
  // each init manager has a chance to initialize its targets.
  init_manager.add(init_target_);
}

void SdsApi::onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                            const std::string&) {
  validateUpdateSize(resources.size());
  auto secret =
      MessageUtil::anyConvert<envoy::api::v2::auth::Secret>(resources[0], validation_visitor_);
  MessageUtil::validate(secret);

  if (secret.name() != sds_config_name_) {
    throw EnvoyException(
        fmt::format("Unexpected SDS secret (expecting {}): {}", sds_config_name_, secret.name()));
  }

  const uint64_t new_hash = MessageUtil::hash(secret);
  if (new_hash != secret_hash_) {
    validateConfig(secret);
    secret_hash_ = new_hash;
    setSecret(secret);
    update_callback_manager_.runCallbacks();
  }

  init_target_.ready();
}

void SdsApi::onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>& resources,
                            const Protobuf::RepeatedPtrField<std::string>&, const std::string&) {
  validateUpdateSize(resources.size());
  Protobuf::RepeatedPtrField<ProtobufWkt::Any> unwrapped_resource;
  *unwrapped_resource.Add() = resources[0].resource();
  onConfigUpdate(unwrapped_resource, resources[0].version());
}

void SdsApi::onConfigUpdateFailed(const EnvoyException*) {
  // We need to allow server startup to continue, even if we have a bad config.
  init_target_.ready();
}

void SdsApi::validateUpdateSize(int num_resources) {
  if (num_resources == 0) {
    throw EnvoyException(
        fmt::format("Missing SDS resources for {} in onConfigUpdate()", sds_config_name_));
  }
  if (num_resources != 1) {
    throw EnvoyException(fmt::format("Unexpected SDS secrets length: {}", num_resources));
  }
}

void SdsApi::initialize() {
  subscription_ = Envoy::Config::SubscriptionFactory::subscriptionFromConfigSource(
      sds_config_, local_info_, dispatcher_, cluster_manager_, random_, stats_,
      Grpc::Common::typeUrl(envoy::api::v2::auth::Secret().GetDescriptor()->full_name()),
      validation_visitor_, api_, *this);
  subscription_->start({sds_config_name_});
}

} // namespace Secret
} // namespace Envoy
