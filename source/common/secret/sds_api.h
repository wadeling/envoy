#pragma once

#include <functional>

#include "envoy/api/api.h"
#include "envoy/api/v2/auth/cert.pb.h"
#include "envoy/api/v2/core/config_source.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/event/dispatcher.h"
#include "envoy/init/manager.h"
#include "envoy/local_info/local_info.h"
#include "envoy/runtime/runtime.h"
#include "envoy/secret/secret_callbacks.h"
#include "envoy/secret/secret_provider.h"
#include "envoy/server/transport_socket_config.h"
#include "envoy/stats/stats.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/common/callback_impl.h"
#include "common/common/cleanup.h"
#include "common/init/target_impl.h"
#include "common/ssl/certificate_validation_context_config_impl.h"
#include "common/ssl/tls_certificate_config_impl.h"

namespace Envoy {
namespace Secret {

/**
 * SDS API implementation that fetches secrets from SDS server via Subscription.
 */
class SdsApi : public Config::SubscriptionCallbacks {
public:
  SdsApi(const LocalInfo::LocalInfo& local_info, Event::Dispatcher& dispatcher,
         Runtime::RandomGenerator& random, Stats::Store& stats,
         Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
         const envoy::api::v2::core::ConfigSource& sds_config, const std::string& sds_config_name,
         std::function<void()> destructor_cb,
         ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api);

  // Config::SubscriptionCallbacks
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>& resources,
                      const std::string& version_info) override;
  void onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>&,
                      const Protobuf::RepeatedPtrField<std::string>&, const std::string&) override;
  void onConfigUpdateFailed(const EnvoyException* e) override;
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::api::v2::auth::Secret>(resource, validation_visitor_)
        .name();
  }

protected:
  // Creates new secrets.
  virtual void setSecret(const envoy::api::v2::auth::Secret&) PURE;
  virtual void validateConfig(const envoy::api::v2::auth::Secret&) PURE;
  Common::CallbackManager<> update_callback_manager_;

private:
  void validateUpdateSize(int num_resources);
  void initialize();
  Init::TargetImpl init_target_;
  const LocalInfo::LocalInfo& local_info_;
  Event::Dispatcher& dispatcher_;
  Runtime::RandomGenerator& random_;
  Stats::Store& stats_;
  Upstream::ClusterManager& cluster_manager_;

  const envoy::api::v2::core::ConfigSource sds_config_;
  std::unique_ptr<Config::Subscription> subscription_;
  const std::string sds_config_name_;

  uint64_t secret_hash_;
  Cleanup clean_up_;
  ProtobufMessage::ValidationVisitor& validation_visitor_;
  Api::Api& api_;
};

class TlsCertificateSdsApi;
class CertificateValidationContextSdsApi;
typedef std::shared_ptr<TlsCertificateSdsApi> TlsCertificateSdsApiSharedPtr;
typedef std::shared_ptr<CertificateValidationContextSdsApi>
    CertificateValidationContextSdsApiSharedPtr;

/**
 * TlsCertificateSdsApi implementation maintains and updates dynamic TLS certificate secrets.
 */
class TlsCertificateSdsApi : public SdsApi, public TlsCertificateConfigProvider {
public:
  static TlsCertificateSdsApiSharedPtr
  create(Server::Configuration::TransportSocketFactoryContext& secret_provider_context,
         const envoy::api::v2::core::ConfigSource& sds_config, const std::string& sds_config_name,
         std::function<void()> destructor_cb) {
    return std::make_shared<TlsCertificateSdsApi>(
        secret_provider_context.localInfo(), secret_provider_context.dispatcher(),
        secret_provider_context.random(), secret_provider_context.stats(),
        secret_provider_context.clusterManager(), *secret_provider_context.initManager(),
        sds_config, sds_config_name, destructor_cb,
        secret_provider_context.messageValidationVisitor(), secret_provider_context.api());
  }

  TlsCertificateSdsApi(const LocalInfo::LocalInfo& local_info, Event::Dispatcher& dispatcher,
                       Runtime::RandomGenerator& random, Stats::Store& stats,
                       Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
                       const envoy::api::v2::core::ConfigSource& sds_config,
                       const std::string& sds_config_name, std::function<void()> destructor_cb,
                       ProtobufMessage::ValidationVisitor& validation_visitor, Api::Api& api)
      : SdsApi(local_info, dispatcher, random, stats, cluster_manager, init_manager, sds_config,
               sds_config_name, destructor_cb, validation_visitor, api) {}

  // SecretProvider
  const envoy::api::v2::auth::TlsCertificate* secret() const override {
    return tls_certificate_secrets_.get();
  }
  Common::CallbackHandle* addUpdateCallback(std::function<void()> callback) override {
    return update_callback_manager_.add(callback);
  }

protected:
  void setSecret(const envoy::api::v2::auth::Secret& secret) override {
    tls_certificate_secrets_ =
        std::make_unique<envoy::api::v2::auth::TlsCertificate>(secret.tls_certificate());
  }
  void validateConfig(const envoy::api::v2::auth::Secret&) override {}

private:
  TlsCertificatePtr tls_certificate_secrets_;
};

/**
 * CertificateValidationContextSdsApi implementation maintains and updates dynamic certificate
 * validation context secrets.
 */
class CertificateValidationContextSdsApi : public SdsApi,
                                           public CertificateValidationContextConfigProvider {
public:
  static CertificateValidationContextSdsApiSharedPtr
  create(Server::Configuration::TransportSocketFactoryContext& secret_provider_context,
         const envoy::api::v2::core::ConfigSource& sds_config, const std::string& sds_config_name,
         std::function<void()> destructor_cb) {
    return std::make_shared<CertificateValidationContextSdsApi>(
        secret_provider_context.localInfo(), secret_provider_context.dispatcher(),
        secret_provider_context.random(), secret_provider_context.stats(),
        secret_provider_context.clusterManager(), *secret_provider_context.initManager(),
        sds_config, sds_config_name, destructor_cb,
        secret_provider_context.messageValidationVisitor(), secret_provider_context.api());
  }
  CertificateValidationContextSdsApi(
      const LocalInfo::LocalInfo& local_info, Event::Dispatcher& dispatcher,
      Runtime::RandomGenerator& random, Stats::Store& stats,
      Upstream::ClusterManager& cluster_manager, Init::Manager& init_manager,
      const envoy::api::v2::core::ConfigSource& sds_config, std::string sds_config_name,
      std::function<void()> destructor_cb, ProtobufMessage::ValidationVisitor& validation_visitor,
      Api::Api& api)
      : SdsApi(local_info, dispatcher, random, stats, cluster_manager, init_manager, sds_config,
               sds_config_name, destructor_cb, validation_visitor, api) {}

  // SecretProvider
  const envoy::api::v2::auth::CertificateValidationContext* secret() const override {
    return certificate_validation_context_secrets_.get();
  }
  Common::CallbackHandle* addUpdateCallback(std::function<void()> callback) override {
    return update_callback_manager_.add(callback);
  }

  Common::CallbackHandle* addValidationCallback(
      std::function<void(const envoy::api::v2::auth::CertificateValidationContext&)> callback) {
    return validation_callback_manager_.add(callback);
  }

protected:
  void setSecret(const envoy::api::v2::auth::Secret& secret) override {
    certificate_validation_context_secrets_ =
        std::make_unique<envoy::api::v2::auth::CertificateValidationContext>(
            secret.validation_context());
  }

  void validateConfig(const envoy::api::v2::auth::Secret& secret) override {
    validation_callback_manager_.runCallbacks(secret.validation_context());
  }

private:
  CertificateValidationContextPtr certificate_validation_context_secrets_;
  Common::CallbackManager<const envoy::api::v2::auth::CertificateValidationContext&>
      validation_callback_manager_;
};

} // namespace Secret
} // namespace Envoy
