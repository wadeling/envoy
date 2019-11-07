#include "common/perf/perf.h"

namespace ENVOY {
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> usType;

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);
    std::vector<int> DuplicateIdArr(MAX_RECORD_NUM);

    int BufferToSmallCount = 0;

    int perfOpen = 0;

    void perfOn() {
        perfOpen = 1;
    }

    void perfOff() {
        perfOpen = 0;
    }

    std::string intToStr(int i) {
        std::ostringstream tmpStream;
        tmpStream << i ;
        return tmpStream.str();
    }

    int checkDuplicateId(int id) {
        if ( LatencyRecordArr[id].server_rcv_time != 0
            && LatencyRecordArr[id].client_send_time !=0
            && LatencyRecordArr[id].client_rcv_time !=0
            && LatencyRecordArr[id].server_send_time !=0 ) {
            DuplicateIdArr[id]++;
            return -1;
        }
        return 0;
    }

    uint64_t getCurrentTime() {
        usType tp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
        return tp.time_since_epoch().count();
//        std::chrono::steady_clock::duration d = std::chrono::steady_clock::now().time_since_epoch();
//        std::chrono::microseconds mic = std::chrono::duration_cast<std::chrono::microseconds>(d);
//        return mic.count();
    }

    int getXId(Envoy::Buffer::Instance& data,const std::string& key) {
        ssize_t pos = data.search(key.c_str(),key.size(),0);
        if (pos == -1) {
//            printf("not find x-id in data.\r\n");
            return -1;
        }
        ssize_t pos2 = data.search(XID_END.c_str(),XID_END.size(),pos);
        if (pos2 == -1) {
//            printf("not find x-id end in data.\r\n");
            return -2;
        }

        char id[64] = {0};
        int start = pos + XID.size() + 1;
        data.copyOut(start,pos2 - start,id );
        return atoi(id);
    }

    int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) {
        // check open flag
        if (perfOpen == 0 ) {
            return 0;
        }
//        return 0;
//        int id= getXId(data,XID_ALTER);
//        if ( id == -1) {
//           // serach another
//           id = getXId(data,XID);
//           if (id == -1) {
//               BufferToSmallCount++;
//                return -1;
//            }
//        }
        int id= getXId(data,XID);
        if ( id == -1) {
            BufferToSmallCount++;
            return -1;
        }
//        printf("get xid %d \r\n",id);

        if (id + 1 >= MAX_RECORD_NUM) {
            return -1;
        }

        //check duplicate
        if (checkDuplicateId(id) != 0)  {
            return -1;
        }

        uint64_t cur_time = getCurrentTime();
//        printf("get cur_time %llu \r\n",cur_time);

        if (type == Server_Rcv_Time) {
            if (LatencyRecordArr[id].server_rcv_time != 0) {
//                printf("change to clint recv time \r\n");
                LatencyRecordArr[id].client_rcv_time = cur_time;
            } else {
                LatencyRecordArr[id].server_rcv_time = cur_time;
            }
        } else if ( type == Server_Send_Time) {
            LatencyRecordArr[id].server_send_time = cur_time;
        } else if ( type == Client_Send_Time) {
            if (LatencyRecordArr[id].client_send_time != 0) {
//                printf("change to server send time \r\n");
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

    void flushToFile(std::string& result,std::string file) {
        // write file
        std::ofstream out(file.c_str());
        if (out.is_open()) {
            out << result;
            out.close();
        } else {
            printf("[error] can not open file %s\r\n",file.c_str());
        }
    }

    std::string dumpLatency(int start,int end,std::string file) {
        std::string result;
        int array_len = int(LatencyRecordArr.size());
        int start_index = start > array_len ? array_len: start;
        int end_index =  end > array_len ? array_len : end;
        if (end_index == 0) {
            end_index = array_len;
        }

        printf("array_len %d,start %d,end %d\r\n",array_len,start_index,end_index);

        result += "id\tserver-rcv-time\tclient-send-time\tclient-rcv-time\tserver-send-time\r\n";
        for (int i = start_index; i < end_index; ++i) {
            std::ostringstream tmpStream;
            tmpStream << i << "\t";
            tmpStream << LatencyRecordArr[i].server_rcv_time << "\t";
            tmpStream << LatencyRecordArr[i].client_send_time << "\t";
            tmpStream << LatencyRecordArr[i].client_rcv_time << "\t";
            tmpStream << LatencyRecordArr[i].server_send_time << "\r\n";
            result += tmpStream.str();
        }
//        result += "dump end";
       printf("result size %lu\r\n",result.length());

        flushToFile(result,file);

        return result;
    }

    int dumpExtra(std::string file) {
        dumpDuplicateId(file) ;
        return 0;
    }

    int dumpDuplicateId(std::string file) {
        std::string result;
        for (int i = 0; i < int(DuplicateIdArr.size()); ++i) {
            if (DuplicateIdArr[i] != 0) {
                result += "id " + intToStr(i) + "duplicate,num " + intToStr(DuplicateIdArr[i]) + "\r\n";
            }
        }
        result += " not found xid num:" + intToStr(BufferToSmallCount) ;
        flushToFile(result,file);
        printf("dump duplicate id end.\r\n");
        return 0;
    }

    void resetSta() {
        for (int i = 0; i < int(LatencyRecordArr.size()); ++i) {
            LatencyRecordArr[i].server_send_time = 0;
            LatencyRecordArr[i].server_rcv_time = 0;
            LatencyRecordArr[i].client_send_time = 0;
            LatencyRecordArr[i].client_rcv_time = 0;
        }
        for (int j = 0; j < int(DuplicateIdArr.size()); ++j) {
            DuplicateIdArr[j] = 0;
        }
        BufferToSmallCount = 0;
    }
}
