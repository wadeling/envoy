#include "common/perf/perf.h"

namespace ENVOY {
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> usType;

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);

    int BufferToSmallCount = 0;

    uint64_t get_current_time() {
        usType tp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
    }

    int record_server_rcv_time(int id) {
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        LatencyRecordArr[id].server_rcv_time = get_current_time();
        return 0;
    }

    int record_server_send_time(int id) {
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        LatencyRecordArr[id].server_send_time = get_current_time();
        return 0;
    }

    int record_client_send_time(int id) {
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        LatencyRecordArr[id].client_send_time = get_current_time();
        return 0;
    }

    int record_client_rcv_time(int id) {
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        LatencyRecordArr[id].client_rcv_time = get_current_time();
        return 0;
    }
}
