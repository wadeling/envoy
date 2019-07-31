#pragma once

#include "envoy/server/filter_config.h"

#include "extensions/filters/http/common/empty_http_filter_config.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace GrpcHttp1Bridge {

/**
 * Config registration for the grpc HTTP1 bridge filter. @see NamedHttpFilterConfigFactory.
 */
class GrpcHttp1BridgeFilterConfig : public Common::EmptyHttpFilterConfig, Logger::Loggable<Logger::Id::filter> {
public:
  GrpcHttp1BridgeFilterConfig()
      : Common::EmptyHttpFilterConfig(HttpFilterNames::get().GrpcHttp1Bridge) {}

  Http::FilterFactoryCb createFilter(const std::string&,
                                     Server::Configuration::FactoryContext&) override;
};

} // namespace GrpcHttp1Bridge
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
