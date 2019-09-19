#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {
namespace DubboFilters {

/**
 * Well-known Dubbo filter names.
 * NOTE: New filters should use the well known name: envoy.filters.dubbo.name.
 */
class DubboFilterNameValues {
public:
  // Router filter
  const std::string ROUTER = "envoy.filters.dubbo.router";
};

using DubboFilterNames = ConstSingleton<DubboFilterNameValues>;

} // namespace DubboFilters
} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
