#pragma once

#include "envoy/server/filter_config.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Common {

/**
 * Common base class for HTTP filter factory registrations. Removes a substantial amount of
 * boilerplate.
 */
template <class ConfigProto>
class PrivateProtoFactoryBase : public Server::Configuration::PrivateProtoNamedHttpFilterConfigFactory {
public:
  Http::PrivateProtoFilterFactoryCb
  createPrivateProtoFilterFactoryFromProto(const Protobuf::Message& proto_config,
                               Server::Configuration::FactoryContext& context) override {
    return createFilterFactoryFromProtoTyped(
        MessageUtil::downcastAndValidate<const ConfigProto&>(proto_config), context);
  }

  std::string name() override { return name_; }

protected:
  PrivateProtoFactoryBase(const std::string& name) : name_(name) {}

private:
  virtual Http::PrivateProtoFilterFactoryCb
  createFilterFactoryFromProtoTyped(const ConfigProto& proto_config,
                                    Server::Configuration::FactoryContext& context) PURE;

  const std::string name_;
};

} // namespace Common
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
