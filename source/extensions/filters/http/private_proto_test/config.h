#pragma once

#include "envoy/server/filter_config.h"

#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace PrivateProto {

class PrivateProtoTestConfig : public Server::Configuration::PrivateProtoNamedHttpFilterConfigFactory, Logger::Loggable<Logger::Id::filter> {
public:

  Http::PrivateProtoFilterFactoryCb createPrivateProtoFilterFactoryFromProto(const Protobuf::Message& config) override;
  std::string name() override { return HttpFilterNames::get().PrivateProtoTest; }
};

} // namespace PrivateProto
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
