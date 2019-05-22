#include "extensions/filters/network/echo/echo.h"

#include "envoy/buffer/buffer.h"
#include "envoy/network/connection.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo {

std::string encapHttpRsp(std::string post_content) {
    std::stringstream stream;
    stream << "HTTP/1.1 200 OK\r\n";
    stream << "Server: nginx/1.6.2\r\n";
    stream << "Content-Type:application/octet-stream\r\n";
    stream << "Connection: close\r\n";
    stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
    stream << "Content-Length:" << post_content.length()<<"\r\n";
    stream << "Connection: close\r\n\r\n";
    if (post_content.length() != 0) {
        stream << post_content.c_str();
    }
    return stream.str() ;
}

Network::FilterStatus EchoFilter::onData(Buffer::Instance& data, bool end_stream) {
  ENVOY_CONN_LOG(trace, "echo: got {} bytes,content {}", read_callbacks_->connection(), data.length(),data.toString());

  std::string rsp;
  std::string content;
  content = "bbb";
  rsp = encapHttpRsp(content);
  absl::string_view rspData=rsp;
  data.drain(data.length());
  data.add(rspData);
  ENVOY_CONN_LOG(trace,"rsp:{},end stream {},data {}",read_callbacks_->connection(),rsp,end_stream,data.toString());

//  read_callbacks_->connection().write(data, end_stream);

  read_callbacks_->connection().write(data, end_stream);
  ASSERT(0 == data.length());
  return Network::FilterStatus::StopIteration;
}

} // namespace Echo
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
