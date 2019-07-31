#include "extensions/filters/http/grpc_http1_bridge/config.h"

#include "envoy/registry/registry.h"

#include "extensions/filters/http/grpc_http1_bridge/http1_bridge_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcHttp1Bridge {

Http::FilterFactoryCb
GrpcHttp1BridgeFilterConfig::createFilter(const std::string&,
                                          Server::Configuration::FactoryContext& factory_context) {
  ENVOY_LOG(debug,"GrpcHttp1BridgeFilterConfig::createFilter");

  return [&factory_context](Http::FilterChainFactoryCallbacks& callbacks) {
    ENVOY_LOG(debug,"grpc add to stream filter");
    callbacks.addStreamFilter(std::make_shared<Http1BridgeFilter>(factory_context.grpcContext()));
  };
}

/**
 * Static registration for the grpc HTTP1 bridge filter. @see RegisterFactory.
 */
REGISTER_FACTORY(GrpcHttp1BridgeFilterConfig, Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace GrpcHttp1Bridge
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
