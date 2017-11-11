#include "http_parser.h"

#include <string>

#include "logger.h"

namespace net_utils {

const size_t kMaxHttpRequsetSize = 128 * 1024;

const std::string kRequestEnd = "\r\n\r\n";
const std::string kLineEnd = "\r\n";

const std::string kChunkedTag = "Transfer-Encoding:";
const std::string kChunkedTagValue = "chunked";

const std::string kLengthTag = "Content-Length:";
const std::string kHostTag = "Host:";

HttpParser::HttpParser() : end_header_pos_(std::string::npos) {}
HttpParser::~HttpParser() {}

HttpParser::AppendResult HttpParser::Append(const std::string& message) {
  if (request_.size() + message.size() > kMaxHttpRequsetSize) {
    FLOGE << "Http request is overflowed";
    return HttpParser::AppendResult::ERROR;
  }
  request_ += message;
  end_header_pos_ = request_.find(kRequestEnd);
  if (end_header_pos_ == std::string::npos) {
    return HttpParser::AppendResult::SUCCESS;
  }

  header_ = request_.substr(0, end_header_pos_ + kRequestEnd.size());
  if (!ParseHeader()) {
    return HttpParser::AppendResult::ERROR;
  }
  request_ = request_.substr(header_.size(), request_.size() - header_.size());

  if (length_ == std::string::npos) {
    ready_request_ = std::move(header_);
    return HttpParser::AppendResult::READY_REQUEST;
  }
  if (request_.size() >= length_) {
    ready_request_ = std::move(header_);
    ready_request_ += request_.substr(0, length_);
    request_ = request_.substr(length_, request_.size() - length_);
    return HttpParser::AppendResult::READY_REQUEST;
  }
  return HttpParser::AppendResult::SUCCESS;
}

bool HttpParser::ParseHeader() {
  ParseLength();
  ParseHostAndPort();
  return host_ != "";
}

std::string HttpParser::GetTagValue(const std::string& tag) const {
  if (end_header_pos_ == std::string::npos) {
    return "";
  }

  size_t tag_pos = request_.find(tag);
  if (tag_pos == std::string::npos || tag_pos > end_header_pos_) {
    return "";
  }
  size_t next_endline_pos = request_.find(kLineEnd, tag_pos);
  size_t pos = tag_pos + tag.size() + 1;
  return request_.substr(pos, next_endline_pos - pos);
}

void HttpParser::ParseHostAndPort() {
  std::string host_and_port = GetTagValue(kHostTag);
  size_t del_pos = host_and_port.find(":");
  if (del_pos == std::string::npos) {
    host_ = host_and_port;
    port_ = "";
    return;
  }
  host_ = host_and_port.substr(0, del_pos);
  port_ = host_and_port.substr(del_pos + 1,
                               host_and_port.size() - 1 - host_.size());
}

void HttpParser::ParseLength() {
  std::string length = GetTagValue(kLengthTag);
  try {
    size_t len = std::stoul(length);
    length_ = len;
  } catch(...) {
    length_ = std::string::npos;
  }
}

bool HttpParser::FindNextHeader() const {
  return (request_.find(kHostTag, end_header_pos_) != std::string::npos);
}

std::string HttpParser::GetNextRequest() {
  std::string ans(std::move(ready_request_));
  ResetFields();
  return ans;
}

std::string HttpParser::GetHost() const {
  return host_;
}

std::string HttpParser::GetPort() const {
  return port_;
}

void HttpParser::ResetFields() {
  end_header_pos_ = std::string::npos;
  length_ = std::string::npos;
  header_ = "";
  ready_request_ = "";
  host_ = "";
  port_ = "";
}

}
