
#include "extensions/filters/http/private_proto_test/private_proto_test.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace PrivateProto {


Http::PrivateProtoFilterDataStatus PrivateProtoTest::decodeData(Buffer::Instance& buff, bool end_stream) {
  ENVOY_LOG(debug,"Private Proto Test decode Data length {},end_stream {}",buff.length(),end_stream);

  return Http::PrivateProtoFilterDataStatus ::Continue;
}

Http::PrivateProtoFilterDataStatus PrivateProtoTest::decodeClientData(Buffer::Instance& buff, bool end_stream) {
  ENVOY_LOG(debug,"Private Proto Test decode Client Data length {},end_stream {}",buff.length(),end_stream);

  return Http::PrivateProtoFilterDataStatus ::Continue;
}

} // namespace PrivateProto
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
