#pragma once

#include <cstdint>
#include <stdlib.h>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <fstream>
#include "envoy/buffer/buffer.h"

#define  MAX_RECORD_NUM     (60*4*60*1000)  // 60 concurrency,4 min,60sec,1000qps

const std::string XID= "x-id";
const std::string XID_ALTER= "X-Id";
const std::string XID_END= "\r\n";

namespace ENVOY {
    enum Record_Type_Enum {
        Server_Rcv_Time = 0,
        Server_Send_Time = 1,
        Client_Send_Time = 2,
        Client_Rcv_Time = 3,
    };

    struct LatencyRecord {
        uint64_t server_rcv_time;
        uint64_t server_send_time; // envoy rsp to client as server
        uint64_t client_send_time; // envoy send to cluster as client
        uint64_t client_rcv_time;
    };

    extern int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) ;

    extern std::string dumpLatency(int start,int end,std::string file) ;

    extern int dumpDuplicateId(std::string file);
    extern int dumpExtra(std::string file);
    extern void resetSta();
    extern void perfOn();
    extern void perfOff();

    extern int BufferToSmallCount;

}
