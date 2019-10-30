#include "perf.h"

namespace ENVOY {

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);

    int ENVOY::BufferToSmallCount = 0;

    int record_server_rcv_time(int id) {
        if (id + 1 > MAX_RECORD_NUM) {
            return -1;
        }
        LatencyRecordArr[id].server_rcv_time = ;
    }
}
