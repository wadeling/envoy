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
    const envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboProxy& proto_config ABSL_ATTRIBUTE_UNUSED,
    Server::Configuration::FactoryContext& context ABSL_ATTRIBUTE_UNUSED) {
  ENVOY_LOG(debug,"dubbo: proxy create filter factory");
  std::shared_ptr<Config> filter_config(std::make_shared<ConfigImpl>(proto_config, context));

  return [filter_config, &context](Http::PrivateProtoFilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addPreSrvDecodeFilter(std::make_shared<ConnectionManager>(
        *filter_config, context.random(), context.dispatcher().timeSource()));
    callbacks.addClientFilter(std::make_shared<ConnectionManager>(
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

// class ConfigImpl.
ConfigImpl::ConfigImpl(const DubboProxyConfig& config,
                       Server::Configuration::FactoryContext& context)
    : context_(context), stats_prefix_(fmt::format("dubbo.{}.", config.stat_prefix())),
      stats_(DubboFilterStats::generateStats(stats_prefix_, context_.scope())),
      serialization_type_(
          SerializationTypeMapper::lookupSerializationType(config.serialization_type())),
      protocol_type_(ProtocolTypeMapper::lookupProtocolType(config.protocol_type())) {
  ENVOY_LOG(debug,"dubbo config impl create");

}

ProtocolPtr ConfigImpl::createProtocol() {
  return NamedProtocolConfigFactory::getFactory(protocol_type_).createProtocol(serialization_type_);
}


} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
