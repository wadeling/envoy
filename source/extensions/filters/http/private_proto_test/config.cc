#include "extensions/filters/http/private_proto_test/config.h"
#include "extensions/filters/http/private_proto_test/private_proto_test.h"

#include "envoy/registry/registry.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace PrivateProto {

Http::PrivateProtoFilterFactoryCb
PrivateProtoTestConfig::createPrivateProtoFilterFactoryFromProto(const Protobuf::Message& config ABSL_ATTRIBUTE_UNUSED) {
  ENVOY_LOG(debug,"PrivateProtoTestConfig::createFilter");

  return [](Http::PrivateProtoFilterChainFactoryCallbacks& callbacks) {
    ENVOY_LOG(debug,"PrivateProto add to filter");
    // add to server filter
    callbacks.addPreSrvDecodeFilter(std::make_shared<PrivateProtoTest>());
    // add client filter
    callbacks.addClientFilter(std::make_shared<PrivateProtoTest>());
  };
}

/**
 * Static registration for the privateProtoTest filter. @see RegisterFactory.
 */
REGISTER_FACTORY(PrivateProtoTestConfig, Server::Configuration::PrivateProtoNamedHttpFilterConfigFactory);

} // namespace PrivateProto
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
