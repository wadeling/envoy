#include "common/perf/perf.h"

namespace Envoy {
    std::list<std::pair<uint64_t,uint64_t> > HeaderCompleteTimeList;
    std::list<std::pair<uint64_t,uint64_t> > ServerMsgCompleteTimeList;
    std::list<std::pair<uint64_t,uint64_t> > ClientMsgCompleteTimeList;
    std::list<std::pair<uint64_t,uint64_t> > StreamDecodeHeaderTimeList;

    uint64_t PendingReqCount = 0 ;
    uint64_t UsingExistConnCount= 0 ;

    std::vector<LatencyRecord> LatencyRecordArr(MAX_RECORD_NUM);
    std::vector<int> DuplicateIdArr(MAX_RECORD_NUM);

    // not find "x-id" 's num
    int BufferToSmallCount = 0;

    // perf switch: 1-on (default),0-of
    int PerfSwitch = 1;
    void perfOn() {
        PerfSwitch = 1;
    }

    void perfOff() {
        PerfSwitch = 0;
    }

    void recordHeaderCompleteTime(std::pair<uint64_t,uint64_t>& t) {
        HeaderCompleteTimeList.push_back(t);
    }

    void recordServerMsgCompleteTime(std::pair<uint64_t,uint64_t>& t) {
        ServerMsgCompleteTimeList.push_back(t);
    }

    void recordClientMsgCompleteTime(std::pair<uint64_t,uint64_t>& t) {
        ClientMsgCompleteTimeList.push_back(t);
    }

    void recordStreamDecodeHeaderTime(std::pair<uint64_t,uint64_t>& t) {
        StreamDecodeHeaderTimeList.push_back(t);
    }

    void incPendingReqCount() {
        PendingReqCount++;
    }
    void incUsingExistConnCount() {
        UsingExistConnCount++;
    }

    int checkDuplicateId(int id) {
        if ( LatencyRecordArr[id].server_rcv_time != 0
            && LatencyRecordArr[id].client_send_time !=0
            && LatencyRecordArr[id].client_rcv_time !=0
            && LatencyRecordArr[id].server_send_time !=0 ) {
            DuplicateIdArr[id]++;
            ENVOY_LOG_MISC(trace,"find duplicate id {}",id);
            return -1;
        }
        return 0;
    }

    int getXId(Envoy::Buffer::Instance& data,const std::string& key) {
//        ENVOY_LOG_MISC(trace,"get xid.data: {},key: {}",data.toString(),key);
        ssize_t pos = data.search(key.c_str(),key.size(),0);
        if (pos == -1) {
            ENVOY_LOG_MISC(trace,"not find x-id");
            return -1;
        }
        ssize_t pos2 = data.search(XID_END.c_str(),XID_END.size(),pos);
        if (pos2 == -1) {
            ENVOY_LOG_MISC(trace,"not find x-id end character");
            return -2;
        }

        char id[64] = {0};
        int start = pos + XID.size() + 1;
        data.copyOut(start,pos2 - start,id );
        return atoi(id);
    }

    int recordTimePoint(Envoy::Buffer::Instance& data,Record_Type_Enum type) {
        if (PerfSwitch == 0) {
            ENVOY_LOG_MISC(trace,"perf switch off,not record time");
            return 0;
        }
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
        if ( id < 0) {
            ENVOY_LOG_MISC(trace,"not find x-id.{}",data.toString());
            BufferToSmallCount++;
            return -1;
        }

        ENVOY_LOG_MISC(trace,"get xid {},record time type {}",id,type);

        if (id + 1 >= MAX_RECORD_NUM) {
            ENVOY_LOG_MISC(trace,"x-id over upper limit ");
            return -1;
        }

        //check duplicate
        if (checkDuplicateId(id) != 0)  {
            return -1;
        }

        uint64_t cur_time = getCurrentTime();
        ENVOY_LOG_MISC(trace,"get cur time {}",cur_time);

        if (type == Server_Rcv_Time) {
            if (LatencyRecordArr[id].server_rcv_time != 0) {
                ENVOY_LOG_MISC(trace,"change to client recv time ");
                LatencyRecordArr[id].client_rcv_time = cur_time;
            } else {
                LatencyRecordArr[id].server_rcv_time = cur_time;
            }
        } else if ( type == Server_Send_Time) {
            LatencyRecordArr[id].server_send_time = cur_time;
        } else if ( type == Client_Send_Time) {
            if (LatencyRecordArr[id].client_send_time != 0) {
                ENVOY_LOG_MISC(trace,"change to server send time ");
                LatencyRecordArr[id].server_send_time = cur_time;
            } else {
                LatencyRecordArr[id].client_send_time = cur_time;
            }
        } else if ( type == Client_Rcv_Time) {
            LatencyRecordArr[id].client_rcv_time = cur_time;
        } else {
            ENVOY_LOG_MISC(error,"time type err {}",type);
            return -2;
        }

        return 0;
    }

    void flushToFile(std::string& result,std::string file,std::string path) {
        std::string outfile;
        if (path.length()) {
            outfile = path + "/" + file;
        } else {
            outfile = file;
        }

        // write file
        std::ofstream out(outfile.c_str());
        if (out.is_open()) {
            out << result;
            out.close();
        } else {
            printf("[error] can not open file %s\r\n",outfile.c_str());
        }
    }

    std::string dumpLatency(int start,int end,std::string file,std::string path) {
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

        flushToFile(result,file,path);

        return result;
    }

    int dumpExtra(std::string file,std::string path) {
        dumpDuplicateId(file,path) ;
        return 0;
    }

    int dumpDuplicateId(std::string file,std::string path) {
        std::string result;
        for (int i = 0; i < int(DuplicateIdArr.size()); ++i) {
            if (DuplicateIdArr[i] != 0) {
                result += "id " + intToStr(i) + "duplicate,num " + intToStr(DuplicateIdArr[i]) + "\r\n";
            }
        }
        result += " not found xid num:" + intToStr(BufferToSmallCount) ;
        flushToFile(result,file,path);
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

    void dumpTimeList(std::string file,std::string path,std::list<std::pair<uint64_t,uint64_t> >& list) {
        uint64_t total_time = 0;
        std::string result;
        std::list<std::pair<uint64_t ,uint64_t > >::iterator iter = list.begin();
        for (; iter != list.end() ; iter++) {
            uint64_t take_time = iter->second - iter->first;
            total_time += take_time;
            result += uint64ToStr(iter->first)+ "\t" + uint64ToStr(iter->second) +  "\t" + uint64ToStr(take_time) + "\r\n";
        }
        double average_time = static_cast<double>(total_time)/ static_cast<double>(list.size());
        result += "total time: " + uint64ToStr(total_time) + " us\r\n";
        result += "count:" + uint64ToStr(list.size()) + " us\r\n";
        result += "average time :"  + doubleToStr(average_time) + "us \r\n";

        flushToFile(result,file,path);
    }

    void dumpHeaderCompleteTime(std::string file,std::string path) {
        dumpTimeList(file,path,HeaderCompleteTimeList);
        printf("dump header complete time end\r\n");
    }

    void dumpServerMsgCompleteTime(std::string file,std::string path) {
        dumpTimeList(file,path,ServerMsgCompleteTimeList);
        printf("dump Server Msg complete time end\r\n");
    }

    void dumpClientMsgCompleteTime(std::string file,std::string path) {
        dumpTimeList(file,path,ClientMsgCompleteTimeList);
        printf("dump client Msg complete time end\r\n");
    }
    void dumpStreamDecodeHeaderTime(std::string file,std::string path) {
        dumpTimeList(file,path,StreamDecodeHeaderTimeList);
        printf("dump Stream Decode Header time end\r\n");
    }

    std::string getPendingReqCount() {
        return uint64ToStr(PendingReqCount);
    }
    std::string getUsingExistConnCount() {
        return uint64ToStr(UsingExistConnCount);
    }
}
