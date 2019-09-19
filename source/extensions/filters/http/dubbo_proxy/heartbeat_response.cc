#include "extensions/filters/http/dubbo_proxy/heartbeat_response.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

DubboFilters::DirectResponse::ResponseType
HeartbeatResponse::encode(MessageMetadata& metadata, DubboProxy::Protocol& protocol,
                          Buffer::Instance& buffer) const {
  ASSERT(metadata.response_status() == ResponseStatus::Ok);
  ASSERT(metadata.message_type() == MessageType::HeartbeatResponse);

  if (!protocol.encode(buffer, metadata, "")) {
    throw EnvoyException("failed to encode heartbeat message");
  }

  ENVOY_LOG(debug, "buffer length {}", buffer.length());
  return DirectResponse::ResponseType::SuccessReply;
}

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
