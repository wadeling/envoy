#pragma once

#include <cstdint>
#include <list>
#include <memory>

#include "envoy/event/deferred_deletable.h"
#include "envoy/event/timer.h"
#include "envoy/http/codec.h"
#include "envoy/http/private_proto_filter.h"
#include "envoy/network/connection.h"
#include "envoy/network/filter.h"
#include "envoy/upstream/upstream.h"

#include "common/common/assert.h"
#include "common/common/linked_object.h"
#include "common/common/logger.h"
#include "common/http/codec_wrappers.h"
#include "common/network/filter_impl.h"

namespace Envoy {
namespace Http {

/**
 * Callbacks specific to a codec client.
 */
class CodecClientCallbacks {
public:
  virtual ~CodecClientCallbacks() {}

  /**
   * Called every time an owned stream is destroyed, whether complete or not.
   */
  virtual void onStreamDestroy() PURE;

  /**
   * Called when a stream is reset by the client.
   * @param reason supplies the reset reason.
   */
  virtual void onStreamReset(StreamResetReason reason) PURE;
};

/**
 * This is an HTTP client that multiple stream management and underlying connection management
 * across multiple HTTP codec types.
 */
class CodecClient : public Logger::Loggable<Logger::Id::client>,
                    public Http::ConnectionCallbacks,
                    public Http::PrivateProtoFilterChainFactory,
                    public Http::PrivateProtoFilterChainFactoryCallbacks,
                    public Network::ConnectionCallbacks,
                    public Event::DeferredDeletable {
public:
  // private proto filter list
  struct ClientStreamFilter : public PrivateProtoFilterCallbacks,LinkedObject<ClientStreamFilter> {
      ClientStreamFilter(CodecClient& codec_client, PrivateProtoFilterSharedPtr filter)
              : codec_client_(codec_client), handle_(filter) {}
      CodecClient& codec_client_;
      PrivateProtoFilterSharedPtr handle_;

      PrivateProtoFilterDataStatus decodeClientData(Buffer::Instance& data, bool end_stream) {
          PrivateProtoFilterDataStatus status = handle_->decodeClientData(data, end_stream);
          return status;
      }

      PrivateProtoFilterDataStatus encodeClientData(Buffer::Instance& data ABSL_ATTRIBUTE_UNUSED, bool end_stream ABSL_ATTRIBUTE_UNUSED) {
          PrivateProtoFilterDataStatus status = handle_->encodeClientData(data, end_stream);
          return status;
      }
      Network::Connection& connection() override ;
  };
  typedef std::unique_ptr<ClientStreamFilter> ClientStreamFilterPtr;
  std::list<ClientStreamFilterPtr> pre_client_filters_;
  Http::PrivateProtoFilterFactoriesList pre_client_factories_list_;

  void setPrivateProtoFilterFactoriesList(const PrivateProtoFilterFactoriesList);

  // init all registerd filters
  void createPreSrvFilterChain(Http::PrivateProtoFilterChainFactoryCallbacks& callbacks) override;

  // add filter to filter list
  void addClientFilter(Http::PrivateProtoFilterSharedPtr filter) override ;
  void addPreSrvDecodeFilter(Http::PrivateProtoFilterSharedPtr filter ABSL_ATTRIBUTE_UNUSED) override {}
  void decodePrivateProtoData(Buffer::Instance& data, bool end_stream) ;
  //wrapper for filter callbacks
//  struct privateProtoFilterCallbacks: public PrivateProtoFilterCallbacks {
//      privateProtoFilterCallbacks(CodecClient& codec_client)
//              : codec_client_(codec_client) {}
//      Network::Connection& connection() override ;
//      CodecClient& codec_client_;
//  };
//  typedef std::unique_ptr<privateProtoFilterCallbacks> privateProtoFilterPtr;

  /**
   * Type of HTTP codec to use.
   */
  enum class Type { HTTP1, HTTP2 };

  ~CodecClient();

  /**
   * Add a connection callback to the underlying network connection.
   */
  void addConnectionCallbacks(Network::ConnectionCallbacks& cb) {
    connection_->addConnectionCallbacks(cb);
  }

  /**
   * Close the underlying network connection. This is immediate and will not attempt to flush any
   * pending write data.
   */
  void close();

  /**
   * Send a codec level go away indication to the peer.
   */
  void goAway() { codec_->goAway(); }

  /**
   * @return the underlying connection ID.
   */
  uint64_t id() { return connection_->id(); }

  /**
   * @return the underlying codec protocol.
   */
  Protocol protocol() { return codec_->protocol(); }

  /**
   * @return the underlying connection error.
   */
  absl::string_view connectionFailureReason() { return connection_->transportFailureReason(); }

  /**
   * @return size_t the number of outstanding requests that have not completed or been reset.
   */
  size_t numActiveRequests() { return active_requests_.size(); }

  /**
   * Create a new stream. Note: The CodecClient will NOT buffer multiple requests for HTTP1
   * connections. Thus, calling newStream() before the previous request has been fully encoded
   * is an error. Pipelining is supported however.
   * @param response_decoder supplies the decoder to use for response callbacks.
   * @return StreamEncoder& the encoder to use for encoding the request.
   */
  StreamEncoder& newStream(StreamDecoder& response_decoder);

  void setConnectionStats(const Network::Connection::ConnectionStats& stats) {
    connection_->setConnectionStats(stats);
  }

  void setCodecClientCallbacks(CodecClientCallbacks& callbacks) {
    codec_client_callbacks_ = &callbacks;
  }

  void setCodecConnectionCallbacks(Http::ConnectionCallbacks& callbacks) {
    codec_callbacks_ = &callbacks;
  }

  bool remoteClosed() const { return remote_closed_; }

  Type type() const { return type_; }

protected:
  /**
   * Create a codec client and connect to a remote host/port.
   * @param type supplies the codec type.
   * @param connection supplies the connection to communicate on.
   * @param host supplies the owning host.
   */
  CodecClient(Type type, Network::ClientConnectionPtr&& connection,
              Upstream::HostDescriptionConstSharedPtr host, Event::Dispatcher& dispatcher);

  // Http::ConnectionCallbacks
  void onGoAway() override {
    if (codec_callbacks_) {
      codec_callbacks_->onGoAway();
    }
  }

  void onIdleTimeout() {
    host_->cluster().stats().upstream_cx_idle_timeout_.inc();
    close();
  }

  void disableIdleTimer() {
    if (idle_timer_ != nullptr) {
      idle_timer_->disableTimer();
    }
  }

  void enableIdleTimer() {
    if (idle_timer_ != nullptr) {
      idle_timer_->enableTimer(idle_timeout_.value());
    }
  }

  const Type type_;
  ClientConnectionPtr codec_;
  Network::ClientConnectionPtr connection_;
  Upstream::HostDescriptionConstSharedPtr host_;
  Event::TimerPtr idle_timer_;
  const absl::optional<std::chrono::milliseconds> idle_timeout_;

private:
  /**
   * Wrapper read filter to drive incoming connection data into the codec. We could potentially
   * support other filters in the future.
   */
  struct CodecReadFilter : public Network::ReadFilterBaseImpl {
    CodecReadFilter(CodecClient& parent) : parent_(parent) {}

    // Network::ReadFilter
    Network::FilterStatus onData(Buffer::Instance& data, bool) override {
      parent_.onData(data);
      return Network::FilterStatus::StopIteration;
    }

    CodecClient& parent_;
  };

  struct ActiveRequest;

  /**
   * Wrapper for an outstanding request. Designed for handling stream multiplexing.
   */
  struct ActiveRequest : LinkedObject<ActiveRequest>,
                         public Event::DeferredDeletable,
                         public StreamCallbacks,
                         public StreamDecoderWrapper {
    ActiveRequest(CodecClient& parent, StreamDecoder& inner)
        : StreamDecoderWrapper(inner), parent_(parent) {}

    // StreamCallbacks
    void onResetStream(StreamResetReason reason, absl::string_view) override {
      parent_.onReset(*this, reason);
    }
    void onAboveWriteBufferHighWatermark() override {}
    void onBelowWriteBufferLowWatermark() override {}

    // StreamDecoderWrapper
    void onPreDecodeComplete() override { parent_.responseDecodeComplete(*this); }
    void onDecodeComplete() override {}

    StreamEncoder* encoder_{};
    CodecClient& parent_;
  };

  typedef std::unique_ptr<ActiveRequest> ActiveRequestPtr;

  /**
   * Called when a response finishes decoding. This is called *before* forwarding on to the
   * wrapped decoder.
   */
  void responseDecodeComplete(ActiveRequest& request);

  void deleteRequest(ActiveRequest& request);
  void onReset(ActiveRequest& request, StreamResetReason reason);
  void onData(Buffer::Instance& data);

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent event) override;
  // Pass watermark events from the connection on to the codec which will pass it to the underlying
  // streams.
  void onAboveWriteBufferHighWatermark() override {
    codec_->onUnderlyingConnectionAboveWriteBufferHighWatermark();
  }
  void onBelowWriteBufferLowWatermark() override {
    codec_->onUnderlyingConnectionBelowWriteBufferLowWatermark();
  }

  std::list<ActiveRequestPtr> active_requests_;
  Http::ConnectionCallbacks* codec_callbacks_{};
  CodecClientCallbacks* codec_client_callbacks_{};
  bool connected_{};
  bool remote_closed_{};
};

typedef std::unique_ptr<CodecClient> CodecClientPtr;

/**
 * Production implementation that installs a real codec.
 */
class CodecClientProd :public CodecClient {
public:
  CodecClientProd(Type type, Network::ClientConnectionPtr&& connection,
                  Upstream::HostDescriptionConstSharedPtr host, Event::Dispatcher& dispatcher);
};

} // namespace Http
} // namespace Envoy
