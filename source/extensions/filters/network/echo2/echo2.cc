#include "extensions/filters/network/echo2/echo2.h"

#include "envoy/buffer/buffer.h"
#include "envoy/network/connection.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

    std::stringstream encapHttp(std::string path,std::string host,std::string post_content) {
      std::stringstream stream;
      stream << "POST " << path;
      stream << " HTTP/1.0\r\n";
      stream << "Host: "<< host << "\r\n";
      stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
      stream << "Content-Type:application/octet-stream\n";
      stream << "Content-Length:" << post_content.length()<<"\r\n";
      stream << "Connection:close\r\n\r\n";
      stream << post_content.c_str();
      return stream
    }
Network::FilterStatus Echo2Filter::onData(Buffer::Instance& data, bool end_stream) {
  ENVOY_CONN_LOG(trace, "echo2: got {} bytes", read_callbacks_->connection(), data.length());

  //encap http
  std::stringstream req;
  std::string path;
  path = "/";
  std::string host = "servicename";
  std::string post_content = data.toString();
  req = encapHttp(path,host,post_content);

  //change data


  read_callbacks_->connection().write(data, end_stream);
  ASSERT(0 == data.length());
  return Network::FilterStatus::StopIteration;
}

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
