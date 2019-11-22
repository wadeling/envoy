#pragma once

#include <cstdint>
#include <stdlib.h>
#include <string>
#include <vector>
#include <list>
#include <chrono>
#include <sstream>
#include <fstream>
#include "envoy/buffer/buffer.h"
#include "common/common/logger.h"

#define  MAX_RECORD_NUM     (60*4*60*1000)  // 60 concurrency,4 min,60sec,1000qps

const std::string XID= "x-id";
const std::string XID_ALTER= "X-Id";
const std::string XID_END= "\r\n";

namespace Envoy {

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

    inline std::string intToStr(int i) {
        std::ostringstream tmpStream;
        tmpStream << i ;
        return tmpStream.str();
    }

    inline std::string uint64ToStr(uint64_t i) {
        std::ostringstream tmpStream;
        tmpStream << i ;
        return tmpStream.str();
    }

    inline std::string doubleToStr(double i) {
        std::ostringstream tmpStream;
        tmpStream << i ;
        return tmpStream.str();
    }

    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> usType;
    inline uint64_t getCurrentTime() {
        usType tp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
//        std::chrono::steady_clock::duration d = std::chrono::steady_clock::now().time_since_epoch();
//        std::chrono::microseconds mic = std::chrono::duration_cast<std::chrono::microseconds>(d);
//        return mic.count();
    }

    extern int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) ;

    extern std::string dumpLatency(int start,int end,std::string file,std::string path) ;

    extern int dumpDuplicateId(std::string file,std::string path);
    extern int dumpExtra(std::string file,std::string path);
    extern void resetSta();
    extern void perfOn();
    extern void perfOff();

    extern void recordHeaderCompleteTime(std::pair<uint64_t,uint64_t>& );
    extern void dumpHeaderCompleteTime(std::string file,std::string path);

    extern void recordServerMsgCompleteTime(std::pair<uint64_t,uint64_t>& );
    extern void dumpServerMsgCompleteTime(std::string file,std::string path);

    extern void recordClientMsgCompleteTime(std::pair<uint64_t,uint64_t>& );
    extern void dumpClientMsgCompleteTime(std::string file,std::string path);

    extern int BufferToSmallCount;

}
