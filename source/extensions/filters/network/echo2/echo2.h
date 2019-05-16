#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <http_parser.h>

#include "envoy/network/filter.h"
#include "common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

/**
 * Implementation of a basic echo filter.
 */
class Echo2Filter : public Network::ReadFilter, Logger::Loggable<Logger::Id::filter> {

public:
  Echo2Filter();

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; }
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
  }

  // http parser
  http_parser parser_;

  // htp parser setting
  static http_parser_settings settings_;

  // deal body
  void onBody(const char* data, size_t length) ;

  // header field
  void onHeaderField(const char* data,size_t length);

  // header value
  void onHeaderValue(const char* data, size_t length);

  // header complete
  void onHeadersComplete();

  // dump header
  void dumpHeaders();

private:
  // for http parse: parse state
  enum class HeaderParsingState { Nothing,Field, Value };

  HeaderParsingState header_parsing_state_{HeaderParsingState::Nothing};

  std::string current_header_field_;
  std::string current_header_value_;
  std::map<std::string, std::string> headers_;

  //biz buffer
  std::string biz_buff_;

private:
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace Echo
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
