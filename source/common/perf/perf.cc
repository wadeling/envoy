#include "common/perf/perf.h"

namespace ENVOY {
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> usType;

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);

    int BufferToSmallCount = 0;

    uint64_t getCurrentTime() {
        usType tp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
    }

    int getXId(Envoy::Buffer::Instance& data,const std::string& key) {
        ssize_t pos = data.search(key.c_str(),key.size(),0);
        if (pos == -1) {
            printf("not find x-id in data.\r\n");
            return -1;
        }
        ssize_t pos2 = data.search(XID_END.c_str(),XID_END.size(),pos);
        if (pos2 == -1) {
            printf("not find x-id end in data.\r\n");
            return -2;
        }

        char id[64] = {0};
        int start = pos + XID.size() + 1;
        data.copyOut(start,pos2 - start,id );
        return atoi(id);
    }

    int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) {
        int id= getXId(data,XID_ALTER);
        if ( id == -1) {
           // serach another
           id = getXId(data,XID);
           if (id == -1) {
                return -1;
            }
        }

        printf("get xid %d \r\n",id);

        uint64_t cur_time = getCurrentTime();
        printf("get cur_time %llu \r\n",cur_time);
        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }
        if (type == Server_Rcv_Time) {
            if (LatencyRecordArr[id].server_rcv_time != 0) {
                printf("change to clint recv time \r\n");
                LatencyRecordArr[id].client_rcv_time = cur_time;
            } else {
                LatencyRecordArr[id].server_rcv_time = cur_time;
            }
        } else if ( type == Server_Send_Time) {
            LatencyRecordArr[id].server_send_time = cur_time;
        } else if ( type == Client_Send_Time) {
            if (LatencyRecordArr[id].client_send_time != 0) {
                printf("change to server send time \r\n");
                LatencyRecordArr[id].server_send_time = cur_time;
            } else {
                LatencyRecordArr[id].client_send_time = cur_time;
            }
        } else if ( type == Client_Rcv_Time) {
            LatencyRecordArr[id].client_rcv_time = cur_time;
        } else {
            printf("type err\r\n");
            return -2;
        }

        return 0;
    }

    std::string dumpLatency() {
        std::string result;
        result += "dump latency \r\n";
        result += "id\tserver-rcv-time\tclient-send-time\tclient-rcv-time\tserver-send-time\r\n";
        for (int i = 0; i < 10 ; ++i) {
            std::ostringstream tmpStream;
            tmpStream << i << "\t";
            tmpStream << LatencyRecordArr[i].server_rcv_time << "\t";
            tmpStream << LatencyRecordArr[i].client_send_time << "\t";
            tmpStream << LatencyRecordArr[i].client_rcv_time << "\t";
            tmpStream << LatencyRecordArr[i].server_send_time << "\r\n";
            result += tmpStream.str();
        }
        result += "dump end";
        return result;
    }
}
