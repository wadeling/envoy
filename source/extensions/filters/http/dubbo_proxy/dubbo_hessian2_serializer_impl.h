#pragma once

#include "extensions/filters/http/dubbo_proxy/serializer.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {
class DubboHessian2SerializerImpl : public Serializer {
public:
  ~DubboHessian2SerializerImpl() override = default;
  const std::string& name() const override {
    return ProtocolSerializerNames::get().fromType(ProtocolType::Dubbo, type());
  }
  SerializationType type() const override { return SerializationType::Hessian2; }

  std::pair<RpcInvocationSharedPtr, bool>
  deserializeRpcInvocation(Buffer::Instance& buffer, ContextSharedPtr context) override;

  std::pair<RpcResultSharedPtr, bool> deserializeRpcResult(Buffer::Instance& buffer,
                                                           ContextSharedPtr context) override;

  size_t serializeRpcResult(Buffer::Instance& output_buffer, const std::string& content,
                            RpcResponseType type) override;
};

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
