#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <list>
#include "envoy/network/connection.h"
#include "envoy/http/header_map.h"

namespace Envoy {
namespace Http {

const LowerCaseString PrivateProtoKey("x-private-proto");

class ConnectionManagerImpl;

/**
 * Return codes for encode/decode data filter invocations. The connection manager bases further
 * filter invocations on the return code of the previous filter.
 */
enum class PrivateProtoFilterDataStatus {
    // Continue filter chain iteration.
    Continue,
    // Do not iterate to any of the remaining filters in the chain. Returning
    // FilterDataStatus::Continue from decodeData()/encodeData() or calling
    // continueDecoding()/continueEncoding() MUST be called if continued filter iteration is desired.
    StopIteration,
    // Continue iteration to remaining filters, but ignore any subsequent data or trailers. This
    // results in creating a header only request/response.
    ContinueAndEndStream,
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

  // encode data which will send to upstream
  virtual PrivateProtoFilterDataStatus encodeClientData(Buffer::Instance& data, bool end_stream) PURE;

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

  virtual void addPreSrvDecodeFilter(Http::PrivateProtoFilterSharedPtr filter) PURE;

  virtual void addClientFilter(Http::PrivateProtoFilterSharedPtr filter) PURE;

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
