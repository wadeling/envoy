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
  stream << " HTTP/1.1\r\n";
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
    [](http_parser* parser, const char* at, size_t length) -> int {
        static_cast<Echo2Filter*>(parser->data)->onHeaderField(at, length);
        return 0;
    },
    [](http_parser* parser, const char* at, size_t length) -> int {
        static_cast<Echo2Filter*>(parser->data)->onHeaderValue(at, length);
        return 0;
    },
    [](http_parser* parser) -> int {
        static_cast<Echo2Filter*>(parser->data)->onHeadersComplete();
        return 0;
    }, // on_headerComplete
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
  biz_buff_ = "";
}

void Echo2Filter::onBody(const char *data, size_t length) {
  ENVOY_LOG(trace,"get body len {}, data {}",length,data);

  // save buf
  biz_buff_.append(data,length);
}

void Echo2Filter::onHeaderField(const char* data, size_t length) {
    std::string field(data,length);

    switch(header_parsing_state_){
        case HeaderParsingState::Nothing:
            current_header_field_ = field;
            break;
        case HeaderParsingState::Value:
            headers_[current_header_field_] = current_header_value_;
            current_header_field_= field;
            break;
        case HeaderParsingState::Field:
            current_header_field_.append(field);
            break;
    }
    header_parsing_state_ = HeaderParsingState::Field;
}

void Echo2Filter::onHeaderValue(const char* data, size_t length) {
    const std::string value(data,length);
    switch(header_parsing_state_){
        case HeaderParsingState::Field:
            current_header_value_ = value;
            break;
        case HeaderParsingState::Value:
            current_header_value_.append(value);
            break;
        case HeaderParsingState::Nothing:
            // this shouldn't happen
            ENVOY_LOG(trace,"Internal error in http-parser");
            break;
    }
    header_parsing_state_ = HeaderParsingState::Value;
}

void Echo2Filter::onHeadersComplete() {
    ENVOY_LOG(trace,"header complete");
    dumpHeaders();
    headers_.clear();
}

void Echo2Filter::dumpHeaders() {
    ENVOY_LOG(trace,"dump headers");
    std::map<std::string,std::string>::iterator iter;
    for (iter = headers_.begin(); iter != headers_.end() ; ++iter) {
        ENVOY_LOG(trace,"header field: {},value {}",iter->first,iter->second);
    }
}
Network::FilterStatus Echo2Filter::onData(Buffer::Instance& data, bool end_stream) {
  ENVOY_CONN_LOG(trace, "echo2: got {} bytes", read_callbacks_->connection(), data.length());

  //encap http
  std::string req;
  std::string path;
  path = "/";
  std::string host;
//  host = "servicename";
  host = "10.254.255.7:28194";
  req = encapHttp(path,host,data.toString());

  //add http head to buffer
//  std::string header =  req.str();
//  absl::string_view bufferHeader = req;
//  data.prepend(bufferHeader);

  data.drain(data.length());
  absl::string_view httpData = req;
  data.add(httpData);

  ENVOY_LOG(trace,"http header {},buff {},end_stream",req,data.toString(),end_stream);

  // http parse
//  ssize_t rc = http_parser_execute(&parser_, &settings_, data.toString().c_str(), data.length());
//  if (HTTP_PARSER_ERRNO(&parser_) != HPE_OK && HTTP_PARSER_ERRNO(&parser_) != HPE_PAUSED) {
//      ENVOY_LOG(trace,"parse http err {}",HTTP_PARSER_ERRNO(&parser_));
//  }
//  ENVOY_LOG(trace,"http parse num {},end_stream {}",rc,end_stream);

  //http包解析完成后，onBody里面会解析并保存业务数据。然后这里把业务数据取出来，放到入参data里面，这样流程继续往下执行，最后发到后端服务
//  data.drain(data.length());
//  data.add(biz_buff_);

  return Network::FilterStatus::Continue;
//  read_callbacks_->connection().write(data, end_stream);
//  ASSERT(0 == data.length());

//  return Network::FilterStatus::StopIteration;
}

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
