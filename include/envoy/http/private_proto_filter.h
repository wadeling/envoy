#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <list>
#include "envoy/network/connection.h"

namespace Envoy {
namespace Http {

class ConnectionManagerImpl;

/**
 * Return codes for encode/decode data filter invocations. The connection manager bases further
 * filter invocations on the return code of the previous filter.
 */
enum class PrivateProtoFilterDataStatus {
  // Continue filter chain iteration. If headers have not yet been sent to the next filter, they
  // will be sent first via decodeHeaders()/encodeHeaders(). If data has previously been buffered,
  // the data in this callback will be added to the buffer before the entirety is sent to the next
  // filter.
  Continue,
  // Do not iterate to any of the remaining filters in the chain, and buffer body data for later
  // dispatching. Returning FilterDataStatus::Continue from decodeData()/encodeData() or calling
  // continueDecoding()/continueEncoding() MUST be called if continued filter iteration is desired.
  //
  // This should be called by filters which must parse a larger block of the incoming data before
  // continuing processing and so can not push back on streaming data via watermarks.
  //
  // If buffering the request causes buffered data to exceed the configured buffer limit, a 413 will
  // be sent to the user. On the response path exceeding buffer limits will result in a 500.
  StopIterationAndBuffer,
  // Do not iterate to any of the remaining filters in the chain, and buffer body data for later
  // dispatching. Returning FilterDataStatus::Continue from decodeData()/encodeData() or calling
  // continueDecoding()/continueEncoding() MUST be called if continued filter iteration is desired.
  //
  // This will cause the flow of incoming data to cease until one of the continue.*() functions is
  // called.
  //
  // This should be returned by filters which can nominally stream data but have a transient back-up
  // such as the configured delay of the fault filter, or if the router filter is still fetching an
  // upstream connection.
  StopIterationAndWatermark,
  // Do not iterate to any of the remaining filters in the chain, but do not buffer any of the
  // body data for later dispatching. Returning FilterDataStatus::Continue from
  // decodeData()/encodeData() or calling continueDecoding()/continueEncoding() MUST be called if
  // continued filter iteration is desired.
  StopIterationNoBuffer
};

/**
 * The stream filter callbacks are passed to all filters to use for writing response data and
 * interacting with the underlying stream in general.
 */
class PrivateProtoFilterCallbacks {
public:
  virtual ~PrivateProtoFilterCallbacks() {}

  /**
   * @return const Network::Connection* the originating connection, or nullptr if there is none.
   */
  virtual const Network::Connection* connection() PURE;
};

/**
 * Stream decoder filter callbacks add additional callbacks that allow a decoding filter to restart
 * decoding if they decide to hold data (e.g. for buffering or rate limiting).
 */
class PrivateProtoDecoderFilterCallbacks : public virtual PrivateProtoFilterCallbacks {
public:

  /**
   * Called with data to be encoded, optionally indicating end of stream.
   * @param data supplies the data to be encoded.
   * @param end_stream supplies whether this is the last data frame.
   */
  virtual void encodeData(Buffer::Instance& data, bool end_stream) PURE;

};

/**
 * Common base class for both decoder and encoder filters.
 */
class PrivateProtoFilterBase {
public:
  virtual ~PrivateProtoFilterBase() {}

};

/**
 * Stream decoder filter interface.
 */
class PrivateProtoDecoderFilter : public PrivateProtoFilterBase {
public:

  /**
   * Called with a decoded data frame.
   * @param data supplies the decoded data.
   * @param end_stream supplies whether this is the last data frame.
   * @return FilterDataStatus determines how filter chain iteration proceeds.
   */
  virtual PrivateProtoFilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) PURE;

  virtual PrivateProtoFilterDataStatus decodeClientData(Buffer::Instance& data, bool end_stream) PURE;

  /**
   * Called by the filter manager once to initialize the filter decoder callbacks that the
   * filter should use. Callbacks will not be invoked by the filter after onDestroy() is called.
   */
//  virtual void setDecoderFilterCallbacks(PrivateProtoDecoderFilterCallbacks& callbacks) {}

  /**
   * Called at the end of the stream, when all data has been decoded.
   */
  virtual void decodeComplete() {}
};

typedef std::shared_ptr<PrivateProtoDecoderFilter> PrivateProtoDecoderFilterSharedPtr;

/**
 * Stream encoder filter callbacks add additional callbacks that allow a encoding filter to restart
 * encoding if they decide to hold data (e.g. for buffering or rate limiting).
 */
class PrivateProtoEncoderFilterCallbacks : public virtual PrivateProtoFilterCallbacks {
public:


};

/**
 * Stream encoder filter interface.
 */
class PrivateProtoEncoderFilter : public PrivateProtoFilterBase {
public:

  virtual PrivateProtoFilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) PURE;

};

typedef std::shared_ptr<PrivateProtoEncoderFilter> PrivateProtoEncoderFilterSharedPtr;

/**
 * A filter that handles both encoding and decoding.
 */
class PrivateProtoFilter : public virtual PrivateProtoDecoderFilter, public virtual PrivateProtoEncoderFilter {};

typedef std::shared_ptr<PrivateProtoFilter> PrivateProtoFilterSharedPtr;


class PrivateProtoFilterChainFactoryCallbacks {
public:
  virtual ~PrivateProtoFilterChainFactoryCallbacks() {}

  virtual void addPreSrvDecodeFilter(Http::PrivateProtoDecoderFilterSharedPtr filter) PURE;

  virtual void addClientDecodeFilter(Http::PrivateProtoDecoderFilterSharedPtr filter) PURE;

};

typedef std::function<void(PrivateProtoFilterChainFactoryCallbacks& callbacks)> PrivateProtoFilterFactoryCb;

class PrivateProtoFilterChainFactory {
public:
  virtual ~PrivateProtoFilterChainFactory() {}

  // todo: change name
  virtual void createPreSrvFilterChain(PrivateProtoFilterChainFactoryCallbacks& callbacks) PURE;

};

typedef std::list<Http::PrivateProtoFilterFactoryCb> PrivateProtoFilterFactoriesList;
typedef std::shared_ptr<Http::PrivateProtoFilterFactoriesList> PrivateProtoFilterFactoriesListPtr;
} // namespace Http
} // namespace Envoy
