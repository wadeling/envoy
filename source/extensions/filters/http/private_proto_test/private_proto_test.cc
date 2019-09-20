
#include "extensions/filters/http/private_proto_test/private_proto_test.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace PrivateProto {


Http::PrivateProtoFilterDataStatus PrivateProtoTest::decodeData(Buffer::Instance& buff, bool end_stream) {
  ENVOY_LOG(debug,"Private Proto Test decode Data length {},end_stream {},addr {}",
          buff.length(),end_stream,callbacks_->connection()->localAddress()->asString());

  return Http::PrivateProtoFilterDataStatus ::Continue;
}

Http::PrivateProtoFilterDataStatus PrivateProtoTest::decodeClientData(Buffer::Instance& buff, bool end_stream) {
  ENVOY_LOG(debug,"Private Proto Test decode Client Data length {},end_stream {}",buff.length(),end_stream);

  return Http::PrivateProtoFilterDataStatus ::Continue;
}

Http::PrivateProtoFilterDataStatus PrivateProtoTest::encodeClientData(Buffer::Instance& buff, bool end_stream) {
    ENVOY_LOG(debug,"Private Proto Test encode Client Data length {},end_stream {}",buff.length(),end_stream);

    return Http::PrivateProtoFilterDataStatus ::Continue;
}

Http::PrivateProtoFilterDataStatus PrivateProtoTest::encodeData(Buffer::Instance& buff, bool end_stream) {
    ENVOY_LOG(debug,"Private Proto Test encode Data length {},end_stream {}",buff.length(),end_stream);

    return Http::PrivateProtoFilterDataStatus ::Continue;
}

void PrivateProtoTest::setDecoderFilterCallbacks(Http::PrivateProtoFilterCallbacks& callbacks) {
    callbacks_ = &callbacks;
}

void PrivateProtoTest::setEncoderFilterCallbacks(Http::PrivateProtoFilterCallbacks& callbacks) {
    encode_callbacks_ = &callbacks;
}

} // namespace PrivateProto
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
