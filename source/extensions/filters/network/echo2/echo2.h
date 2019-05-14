#pragma once

#include <string>
#include <iostream>
#include <sstream>
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

private:
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace Echo
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
