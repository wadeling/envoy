#include "extensions/filters/http/dubbo_proxy/conn_manager.h"

#include <cstdint>
//#include <tclDecls.h>

#include "envoy/common/exception.h"

//#include "common/common/fmt.h"
#include "common/http/utility.h"

#include "extensions/filters/http/dubbo_proxy/app_exception.h"
#include "extensions/filters/http/dubbo_proxy/dubbo_hessian2_serializer_impl.h"
//#include "extensions/filters/http/dubbo_proxy/dubbo_protocol_impl.h"
#include "extensions/filters/http/dubbo_proxy/heartbeat_response.h"
#include "extensions/filters/http/dubbo_proxy/serializer_impl.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace DubboProxy {

ConnectionManager::ConnectionManager(Config& config, Runtime::RandomGenerator& random_generator,
                                     TimeSource& time_system)
    : config_(config), time_system_(time_system), stats_(config_.stats()),
      random_generator_(random_generator), protocol_(config.createProtocol()),
      decoder_(std::make_unique<RequestDecoder>(*protocol_, *this)) {}

Http::PrivateProtoFilterDataStatus ConnectionManager::decodeData(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(debug,"dubbo: decode server data, {}", static_cast<void*>(this));
  Network::FilterStatus  status = onData(data,end_stream);
  if (status == Network::FilterStatus::StopIteration) {
      return Http::PrivateProtoFilterDataStatus::StopIteration;
  }
  return Http::PrivateProtoFilterDataStatus::Continue;
}

Http::PrivateProtoFilterDataStatus ConnectionManager::decodeClientData(Buffer::Instance& data, bool end_stream) {
    ENVOY_LOG(debug,"dubbo: decode client data,{}", static_cast<void*>(this));
    setIsResponse(true);

    Network::FilterStatus  status = onData(data,end_stream);
    if (status == Network::FilterStatus::StopIteration) {
        return Http::PrivateProtoFilterDataStatus::StopIteration;
    }
    return Http::PrivateProtoFilterDataStatus::Continue;
}

Http::PrivateProtoFilterDataStatus ConnectionManager::encodeClientData(Buffer::Instance& buff, bool end_stream) {
    ENVOY_LOG(debug,"Private Proto dubbo encode Client Data length {},end_stream {}",buff.length(),end_stream);

    return Http::PrivateProtoFilterDataStatus ::Continue;
}

Http::PrivateProtoFilterDataStatus ConnectionManager::encodeData(Buffer::Instance& buff, bool end_stream) {
    ENVOY_LOG(debug,"Private Proto dubbo encode Data length {},end_stream {}",buff.length(),end_stream);

    return Http::PrivateProtoFilterDataStatus ::Continue;
}

void ConnectionManager::setDecoderFilterCallbacks(Http::PrivateProtoFilterCallbacks& callbacks) {
   private_proto_decoder_filter_callbacks_ = &callbacks;
}

void ConnectionManager::setEncoderFilterCallbacks(Http::PrivateProtoFilterCallbacks& callbacks) {
    private_proto_encoder_filter_callbacks_ = &callbacks;
}
void ConnectionManager::setIsResponse(bool is_response) {
    is_response_ = is_response;
}

Network::FilterStatus ConnectionManager::onData(Buffer::Instance& data, bool end_stream) {

  ENVOY_LOG(trace, "dubbo: read {} bytes", data.length());
  request_buffer_.move(data);

  dispatch();

  //after dispath,put http buff to data
  data.prepend(http_buffer_);
  http_buffer_.drain(http_buffer_.length());
  ENVOY_LOG(trace, "dubbo: after encap to http,data length {},http buff length {},data: {} ",
          data.length(),http_buffer_.length(),data.toString());

  if (end_stream) {
    ENVOY_CONN_LOG(trace, "downstream half-closed", private_proto_decoder_filter_callbacks_->connection());

    // Downstream has closed. Unless we're waiting for an upstream connection to complete a oneway
    // request, close. The special case for oneway requests allows them to complete before the
    // ConnectionManager is destroyed.
    if (stopped_) {
      ASSERT(!active_message_list_.empty());
      auto metadata = (*active_message_list_.begin())->metadata();
      if (metadata && metadata->message_type() == MessageType::Oneway) {
        ENVOY_CONN_LOG(trace, "waiting for one-way completion", private_proto_decoder_filter_callbacks_->connection());
        half_closed_ = true;
        return Network::FilterStatus::StopIteration;
      }
    }

    ENVOY_LOG(debug, "dubbo: end data processing");
    resetAllMessages(false);
    private_proto_decoder_filter_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  }

  return Network::FilterStatus::StopIteration;
}

StreamHandler& ConnectionManager::newStream() {
  ENVOY_LOG(debug, "dubbo: create the new decoder event handler");

  ActiveMessagePtr new_message(std::make_unique<ActiveMessage>(*this));
  new_message->moveIntoList(std::move(new_message), active_message_list_);
  return **active_message_list_.begin();
}

Network::Connection& ConnectionManager::connection() {
    return private_proto_decoder_filter_callbacks_->connection();
}

void ConnectionManager::onHeartbeat(MessageMetadataSharedPtr metadata) {
  stats_.request_event_.inc();

  if (private_proto_decoder_filter_callbacks_->connection().state() != Network::Connection::State::Open) {
    ENVOY_LOG(warn, "dubbo: downstream connection is closed or closing");
    return;
  }

  metadata->setResponseStatus(ResponseStatus::Ok);
  metadata->setMessageType(MessageType::HeartbeatResponse);

  HeartbeatResponse heartbeat;
  Buffer::OwnedImpl response_buffer;
  heartbeat.encode(*metadata, *protocol_, response_buffer);

  private_proto_decoder_filter_callbacks_->connection().write(response_buffer, false);
}

void ConnectionManager::dispatch() {
  if (0 == request_buffer_.length()) {
    ENVOY_LOG(warn, "dubbo: it's empty data");
    return;
  }

  if (stopped_) {
    ENVOY_CONN_LOG(debug, "dubbo: dubbo filter stopped", private_proto_decoder_filter_callbacks_->connection());
    return;
  }

  decoder_->setIsResponse(is_response_);
  try {
    bool underflow = false;
    while (!underflow) {
      decoder_->onData(request_buffer_, underflow,http_buffer_,private_proto_decoder_filter_callbacks_->connection());
    }
    return;
  } catch (const EnvoyException& ex) {
    ENVOY_CONN_LOG(error, "dubbo error: {}", private_proto_decoder_filter_callbacks_->connection(), ex.what());
    private_proto_decoder_filter_callbacks_->connection().close(Network::ConnectionCloseType::NoFlush);
    stats_.request_decoding_error_.inc();
  }
  resetAllMessages(true);
}

void ConnectionManager::sendLocalReply(MessageMetadata& metadata,
                                       const DubboFilters::DirectResponse& response,
                                       bool end_stream) {
  if (read_callbacks_->connection().state() != Network::Connection::State::Open) {
    return;
  }

  DubboFilters::DirectResponse::ResponseType result =
      DubboFilters::DirectResponse::ResponseType::ErrorReply;

  try {
    Buffer::OwnedImpl buffer;
    result = response.encode(metadata, *protocol_, buffer);
    read_callbacks_->connection().write(buffer, end_stream);
  } catch (const EnvoyException& ex) {
    ENVOY_CONN_LOG(error, "dubbo error: {}", read_callbacks_->connection(), ex.what());
  }

  if (end_stream) {
    read_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  }

  switch (result) {
  case DubboFilters::DirectResponse::ResponseType::SuccessReply:
    stats_.local_response_success_.inc();
    break;
  case DubboFilters::DirectResponse::ResponseType::ErrorReply:
    stats_.local_response_error_.inc();
    break;
  case DubboFilters::DirectResponse::ResponseType::Exception:
    stats_.local_response_business_exception_.inc();
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

void ConnectionManager::continueDecoding() {
  ENVOY_CONN_LOG(debug, "dubbo filter continued", read_callbacks_->connection());
  stopped_ = false;
  dispatch();

  if (!stopped_ && half_closed_) {
    // If we're half closed, but not stopped waiting for an upstream,
    // reset any pending rpcs and close the connection.
    resetAllMessages(false);
    read_callbacks_->connection().close(Network::ConnectionCloseType::FlushWrite);
  }
}

void ConnectionManager::deferredMessage(ActiveMessage& message) {
  if (!message.inserted()) {
    return;
  }
  read_callbacks_->connection().dispatcher().deferredDelete(
      message.removeFromList(active_message_list_));
}

void ConnectionManager::resetAllMessages(bool local_reset) {
  while (!active_message_list_.empty()) {
    if (local_reset) {
      ENVOY_CONN_LOG(debug, "local close with active request", private_proto_decoder_filter_callbacks_->connection());
      stats_.cx_destroy_local_with_active_rq_.inc();
    } else {
      ENVOY_CONN_LOG(debug, "remote close with active request", private_proto_decoder_filter_callbacks_->connection());
      stats_.cx_destroy_remote_with_active_rq_.inc();
    }

    active_message_list_.front()->onReset();
  }
}

} // namespace DubboProxy
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
