#include "extensions/filters/http/dubbo_proxy/buffer_helper.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

double BufferHelper::peekDouble(Buffer::Instance& buffer, uint64_t offset) {
  if (buffer.length() < offset + 8) {
    throw EnvoyException("buffer underflow");
  }
  double i;
  uint64_t j = buffer.peekBEInt<uint64_t>(offset);
  std::memcpy(&i, &j, 8);
  return i;
}

float BufferHelper::peekFloat(Buffer::Instance& buffer, uint64_t offset) {
  if (buffer.length() < offset + 4) {
    throw EnvoyException("buffer underflow");
  }
  float i;
  uint32_t j = buffer.peekBEInt<uint32_t>(offset);
  std::memcpy(&i, &j, 4);
  return i;
}
} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
