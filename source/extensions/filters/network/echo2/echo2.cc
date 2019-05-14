#include "extensions/filters/network/echo2/echo2.h"

#include "envoy/buffer/buffer.h"
#include "envoy/network/connection.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

std::string encapHttp(std::string path,std::string host,std::string post_content) {
  std::stringstream stream;
  stream << "POST " << path;
  stream << " HTTP/1.0\r\n";
  stream << "Host: "<< host << "\r\n";
  stream << "User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
  stream << "X-Test: haha\r\n";
  stream << "Content-Type:application/octet-stream\n";
  stream << "Content-Length:" << post_content.length()<<"\r\n";
  stream << "Connection:close\r\n\r\n";
  if (post_content.length() != 0) {
    stream << post_content.c_str();
  }
  return stream.str() ;
}

http_parser_settings Echo2Filter::settings_{
    nullptr, // on_message_begin
    nullptr, // on_url
    nullptr, // on_status
    nullptr, // on_header_fileld
    nullptr, // on_header_value
    nullptr, // on_headerComplete
    [](http_parser* parser, const char* at, size_t length) -> int {
        static_cast<Echo2Filter*>(parser->data)->onBody(at, length);
        return 0;
    },
    nullptr, // on_message_complete
    nullptr, // on_chunk_header
    nullptr  // on_chunk_complete
};

Echo2Filter::Echo2Filter() {
  http_parser_init(&parser_,HTTP_REQUEST);
  parser_.data = this;

}

void Echo2Filter::onBody(const char *data, size_t length) {
  ENVOY_LOG(trace,"get body len {}, data {}",length,data);
}

Network::FilterStatus Echo2Filter::onData(Buffer::Instance& data, bool end_stream) {
  ENVOY_CONN_LOG(trace, "echo2: got {} bytes", read_callbacks_->connection(), data.length());

  //encap http
  std::string req;
  std::string path;
  path = "/";
  std::string host;
  host = "servicename";
  req = encapHttp(path,host,data.toString());

  //add http head to buffer
//  std::string header =  req.str();
//  absl::string_view bufferHeader = req;
//  data.prepend(bufferHeader);

  data.drain(data.length());
  absl::string_view httpData = req;
  data.add(httpData);

  ENVOY_LOG(trace,"http header {},buff {}",req,data.toString());

  // http parse
  ssize_t rc = http_parser_execute(&parser_, &settings_, data.toString().c_str(), data.length());
  if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK && HTTP_PARSER_ERRNO(&parser_) != HPE_PAUSED) {
      ENVOY_LOG(trace,"parse http err {}",HTTP_PARSER_ERRNO(&parser_));
  }
  ENVOY_LOG(trace,"http parse num {}",rc);

  read_callbacks_->connection().write(data, end_stream);
  ASSERT(0 == data.length());

  return Network::FilterStatus::StopIteration;
}

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
