#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

#define  MAX_RECORD_NUM     (60*4*60*1000)  // 60 concurrency,4 min,60sec,1000qps
namespace ENVOY {

    struct LatencyRecord {
        uint64_t server_rcv_time;
        uint64_t server_send_time; // envoy rsp to client as server
        uint64_t client_send_time; // envoy send to cluster as client
        uint64_t client_rcv_time;
    };

    extern int BufferToSmallCount;
    extern int record_server_rcv_time(int);
    extern int record_server_send_time(int);
    extern int record_client_rcv_time(int);
    extern int record_client_send_time(int);
}
