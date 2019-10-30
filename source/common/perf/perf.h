#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define  MAX_RECORD_NUM     (60*4*60*1000)  // 60 concurrency,4 min,60sec,1000qps
namespace ENVOY {

    struct LatencyRecord {
        int server_rcv_time;
        int server_send_time; // envoy rsp to client as server
        int client_send_time; // envoy send to cluster as client
        int client_rcv_time;
    };

    extern int BufferToSmallCount;
    int record_server_rcv_time(int);
}
