#pragma once

#include "envoy/http/private_proto_filter.h"
#include "envoy/upstream/cluster_manager.h"
#include "common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace PrivateProto {

class PrivateProtoTest: public Logger::Loggable<Logger::Id::filter>,
                        public Http::PrivateProtoFilter {
public:

  Http::PrivateProtoFilterDataStatus decodeData(Buffer::Instance&, bool) override ;

  Http::PrivateProtoFilterDataStatus decodeClientData(Buffer::Instance&, bool) override ;

  Http::PrivateProtoFilterDataStatus encodeClientData(Buffer::Instance&, bool) override ;

  Http::PrivateProtoFilterDataStatus encodeData(Buffer::Instance&, bool) override ;

};

} // namespace PrivateProto
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
