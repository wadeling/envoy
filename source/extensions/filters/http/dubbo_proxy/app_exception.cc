#include "extensions/filters/http/dubbo_proxy/app_exception.h"

#include "common/buffer/buffer_impl.h"

#include "extensions/filters/http/dubbo_proxy/message.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

DownstreamConnectionCloseException::DownstreamConnectionCloseException(const std::string& what)
    : EnvoyException(what) {}

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
