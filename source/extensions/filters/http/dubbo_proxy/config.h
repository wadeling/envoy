#pragma once

#include <string>

#include "envoy/config/filter/network/dubbo_proxy/v2alpha1/dubbo_proxy.pb.h"
#include "envoy/config/filter/network/dubbo_proxy/v2alpha1/dubbo_proxy.pb.validate.h"
#include "extensions/filters/http/common/generic_proto_factory_base.h"
#include "extensions/filters/http/dubbo_proxy/conn_manager.h"
#include "extensions/filters/http/dubbo_proxy/filters/filter.h"
//#include "extensions/filters/http/dubbo_proxy/router/route_matcher.h"
//#include "extensions/filters/http/dubbo_proxy/router/router_impl.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

/**
 * Config registration for the dubbo proxy filter. @see NamedNetworkFilterConfigFactory.
 */
class DubboProxyFilterConfigFactory
    : public Common::PrivateProtoFactoryBase<envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboProxy>,
            Logger::Loggable<Logger::Id::filter>{
public:
  DubboProxyFilterConfigFactory() : PrivateProtoFactoryBase(HttpFilterNames::get().DubboProxy) {}

 Http::PrivateProtoFilterFactoryCb  createFilterFactoryFromProtoTyped(
            const envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboProxy& proto_config,
            Server::Configuration::FactoryContext& context) override;
};

class ConfigImpl : public Config,
                   Logger::Loggable<Logger::Id::config> {
public:
  using DubboProxyConfig = envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboProxy;
  using DubboFilterConfig = envoy::config::filter::network::dubbo_proxy::v2alpha1::DubboFilter;

  ConfigImpl(const DubboProxyConfig& config, Server::Configuration::FactoryContext& context);
  ~ConfigImpl() override = default;

  // Config
  DubboFilterStats& stats() override { return stats_; }
  ProtocolPtr createProtocol() override;

private:

  Server::Configuration::FactoryContext& context_;
  const std::string stats_prefix_;
  DubboFilterStats stats_;
  const SerializationType serialization_type_;
  const ProtocolType protocol_type_;

};

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
