#include "extensions/filters/http/dubbo_proxy/config.h"

#include "envoy/registry/registry.h"

#include "common/config/utility.h"

#include "extensions/filters/http/dubbo_proxy/conn_manager.h"
#include "extensions/filters/http/dubbo_proxy/filters/factory_base.h"
#include "extensions/filters/http/dubbo_proxy/filters/well_known_names.h"
#include "extensions/filters/http/dubbo_proxy/stats.h"

#include "absl/container/flat_hash_map.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

Http::PrivateProtoFilterFactoryCb DubboProxyFilterConfigFactory::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboProxy& proto_config,
    Server::Configuration::FactoryContext& context) {

  std::shared_ptr<Config> filter_config(std::make_shared<ConfigImpl>(proto_config, context));

  return [filter_config, &context](Http::PrivateProtoFilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addPreSrvDecodeFilter(std::make_shared<ConnectionManager>(
        *filter_config, context.random(), context.dispatcher().timeSource()));
  };
}

/**
 * Static registration for the dubbo filter. @see RegisterFactory.
 */
REGISTER_FACTORY(DubboProxyFilterConfigFactory,
                 Server::Configuration::PrivateProtoNamedHttpFilterConfigFactory);

class ProtocolTypeMapper {
public:
  using ConfigProtocolType = envoy::config::filter::network::dubbo_proxy::v2alpha1::ProtocolType;
  using ProtocolTypeMap = absl::flat_hash_map<ConfigProtocolType, ProtocolType>;

  static ProtocolType lookupProtocolType(ConfigProtocolType config_type) {
    const auto& iter = protocolTypeMap().find(config_type);
    ASSERT(iter != protocolTypeMap().end());
    return iter->second;
  }

private:
  static const ProtocolTypeMap& protocolTypeMap() {
    CONSTRUCT_ON_FIRST_USE(ProtocolTypeMap, {
                                                {ConfigProtocolType::Dubbo, ProtocolType::Dubbo},
                                            });
  }
};

class SerializationTypeMapper {
public:
  using ConfigSerializationType =
      envoy::config::filter::network::dubbo_proxy::v2alpha1::SerializationType;
  using SerializationTypeMap = absl::flat_hash_map<ConfigSerializationType, SerializationType>;

  static SerializationType lookupSerializationType(ConfigSerializationType type) {
    const auto& iter = serializationTypeMap().find(type);
    ASSERT(iter != serializationTypeMap().end());
    return iter->second;
  }

private:
  static const SerializationTypeMap& serializationTypeMap() {
    CONSTRUCT_ON_FIRST_USE(SerializationTypeMap,
                           {
                               {ConfigSerializationType::Hessian2, SerializationType::Hessian2},
                           });
  }
};

class RouteMatcherTypeMapper {
public:
  using ConfigProtocolType = envoy::config::filter::network::dubbo_proxy::v2alpha1::ProtocolType;
  using RouteMatcherTypeMap = absl::flat_hash_map<ConfigProtocolType, Router::RouteMatcherType>;

  static Router::RouteMatcherType lookupRouteMatcherType(ConfigProtocolType type) {
    const auto& iter = routeMatcherTypeMap().find(type);
    ASSERT(iter != routeMatcherTypeMap().end());
    return iter->second;
  }

private:
  static const RouteMatcherTypeMap& routeMatcherTypeMap() {
    CONSTRUCT_ON_FIRST_USE(RouteMatcherTypeMap,
                           {
                               {ConfigProtocolType::Dubbo, Router::RouteMatcherType::Default},
                           });
  }
};

// class ConfigImpl.
ConfigImpl::ConfigImpl(const DubboProxyConfig& config,
                       Server::Configuration::FactoryContext& context)
    : context_(context), stats_prefix_(fmt::format("dubbo.{}.", config.stat_prefix())),
      stats_(DubboFilterStats::generateStats(stats_prefix_, context_.scope())),
      serialization_type_(
          SerializationTypeMapper::lookupSerializationType(config.serialization_type())),
      protocol_type_(ProtocolTypeMapper::lookupProtocolType(config.protocol_type())) {
  auto type = RouteMatcherTypeMapper::lookupRouteMatcherType(config.protocol_type());
  route_matcher_ = Router::NamedRouteMatcherConfigFactory::getFactory(type).createRouteMatcher(
      config.route_config(), context);
  if (config.dubbo_filters().empty()) {
    ENVOY_LOG(debug, "using default router filter");

    envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboFilter router_config;
    router_config.set_name(DubboFilters::DubboFilterNames::get().ROUTER);
    registerFilter(router_config);
  } else {
    for (const auto& filter_config : config.dubbo_filters()) {
      registerFilter(filter_config);
    }
  }
}

void ConfigImpl::createFilterChain(DubboFilters::FilterChainFactoryCallbacks& callbacks) {
  for (const DubboFilters::FilterFactoryCb& factory : filter_factories_) {
    factory(callbacks);
  }
}

Router::RouteConstSharedPtr ConfigImpl::route(const MessageMetadata& metadata,
                                              uint64_t random_value) const {
  return route_matcher_->route(metadata, random_value);
}

ProtocolPtr ConfigImpl::createProtocol() {
  return NamedProtocolConfigFactory::getFactory(protocol_type_).createProtocol(serialization_type_);
}

void ConfigImpl::registerFilter(const DubboFilterConfig& proto_config) {
  const std::string& string_name = proto_config.name();

  ENVOY_LOG(debug, "    dubbo filter #{}", filter_factories_.size());
  ENVOY_LOG(debug, "      name: {}", string_name);

  const Json::ObjectSharedPtr filter_config =
      MessageUtil::getJsonObjectFromMessage(proto_config.config());
  ENVOY_LOG(debug, "      config: {}", filter_config->asJsonString());

  auto& factory =
      Envoy::Config::Utility::getAndCheckFactory<DubboFilters::NamedDubboFilterConfigFactory>(
          string_name);
  ProtobufTypes::MessagePtr message = factory.createEmptyConfigProto();
  Envoy::Config::Utility::translateOpaqueConfig(proto_config.config(),
                                                ProtobufWkt::Struct::default_instance(),
                                                context_.messageValidationVisitor(), *message);
  DubboFilters::FilterFactoryCb callback =
      factory.createFilterFactoryFromProto(*message, stats_prefix_, context_);

  filter_factories_.push_back(callback);
}

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy