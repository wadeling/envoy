#include "common/perf/perf.h"

namespace ENVOY {
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> usType;

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);

    int BufferToSmallCount = 0;

    uint64_t getCurrentTime() {
        usType tp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
    }

    int getXId(Envoy::Buffer::Instance& data) {
        ssize_t pos = data.search(XID.c_str(),XID.size(),0);
        if (pos == -1) {
            return -1;
        }
        ssize_t pos2 = data.search(XID_END.c_str(),XID_END.size(),pos);
        if (pos2 == -1) {
            return -1;
        }

        char id[64] = {0};
        int start = pos + XID.size() + 1;
        data.copyOut(start,pos2 - start,id );
        return atoi(id);
    }

    int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) {
        int id = getXId(data);
        if (id == -1) {
            return -1;
        }

        uint64_t cur_time = getCurrentTime();
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        if (type == Server_Rcv_Time) {
            LatencyRecordArr[id].server_rcv_time = cur_time;
        } else if ( type == Server_Send_Time) {
            LatencyRecordArr[id].server_send_time = cur_time;
        } else if ( type == Client_Send_Time) {
            LatencyRecordArr[id].client_send_time = cur_time;
        } else if ( type == Client_Rcv_Time) {
            LatencyRecordArr[id].client_rcv_time = cur_time;
        } else {
            return -2;
        }

        return 0;
    }

}
