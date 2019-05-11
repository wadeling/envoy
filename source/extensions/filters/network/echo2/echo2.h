#pragma once

#include <string>
#include <iostream>
#include <sstream>
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
  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; }
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
  }

private:
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace Echo
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
