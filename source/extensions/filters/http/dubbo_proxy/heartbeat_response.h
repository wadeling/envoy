#pragma once

#include "extensions/filters/http/dubbo_proxy/filters/filter.h"
#include "extensions/filters/http/dubbo_proxy/metadata.h"
#include "extensions/filters/http/dubbo_proxy/protocol.h"
#include "extensions/filters/http/dubbo_proxy/serializer.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

struct HeartbeatResponse : public DubboFilters::DirectResponse,
                           Logger::Loggable<Logger::Id::dubbo> {
  HeartbeatResponse() = default;
  ~HeartbeatResponse() override = default;

  using ResponseType = DubboFilters::DirectResponse::ResponseType;
  ResponseType encode(MessageMetadata& metadata, Protocol& protocol,
                      Buffer::Instance& buffer) const override;
};

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
