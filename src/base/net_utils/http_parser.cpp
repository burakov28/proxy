#include "http_parser.h"

#include <string>

#include "logger.h"

namespace net_utils {

const size_t kMaxHttpRequsetSize = 128 * 1024;

const std::string kRequestEnd = "\r\n\r\n";
const std::string kLineEnd = "\r\n";

const std::string kLengthTag = "Content-Length:";
const std::string kHostTag = "Host:";

const std::string kConnectionTypeTag = "Connection:";
const std::string kConnectionValueClose = "close";

HttpParser::HttpParser(bool is_request) : header_(), host_(), port_(), request_(),
                           length_(std::string::npos),
                           end_header_pos_(std::string::npos),
                           is_request_(is_request) {}

HttpParser::~HttpParser() {}

HttpParser::AppendResult HttpParser::Append(const std::string& message) {
  if (header_.size() + request_.size() + message.size() > kMaxHttpRequsetSize) {
    FLOGE << "Http request is overflowed";
    return HttpParser::AppendResult::ERROR;
  }

  request_ += message;

  if (end_header_pos_ == std::string::npos) {
    end_header_pos_ = request_.find(kRequestEnd);
    if (end_header_pos_ == std::string::npos) {
      return HttpParser::AppendResult::SUCCESS;
    }

    header_ = request_.substr(0, end_header_pos_ + kRequestEnd.size());
    request_.erase(0, header_.size());
    if (is_request_) {
      if (!ParseHeader()) {
        return HttpParser::AppendResult::ERROR;
      }
    }

    if (is_request_ && length_ == std::string::npos) {  // Only header
      return HttpParser::AppendResult::READY_REQUEST;
    }
  }


  if (length_ != std::string::npos && request_.size() >= length_) {  // Header with body
    return HttpParser::AppendResult::READY_REQUEST;
  }
  return HttpParser::AppendResult::SUCCESS;
}

void HttpParser::InsertTag(const std::string& tag_name, const std::string& value) {
  size_t first_endl_pos = header_.find(kLineEnd, 0);
  std::string to_insert = tag_name + " " + value + kLineEnd;
  header_.insert(first_endl_pos + 2, to_insert);
}

void HttpParser::ReplaceTag(const std::string& tag_name, const std::string& value, size_t pos) {
  size_t next_endl = header_.find(kLineEnd, pos);
  header_.erase(pos, next_endl + 2 - pos);

  InsertTag(tag_name, value);
}

void HttpParser::SetTag(const std::string& tag_name, const std::string& value) {
  if (end_header_pos_ == std::string::npos) {
    return;
  }

  size_t pos = header_.find(tag_name, 0);
  if (pos == std::string::npos) {
    InsertTag(tag_name, value);
  } else {
    ReplaceTag(tag_name, value, pos);
  }
}

void HttpParser::ModifyHeader() {
  if (end_header_pos_ == std::string::npos) {
    return;
  }

  std::string to_remove = "http://" + host_;

  size_t del_pos = header_.find(to_remove, 0);
  if (del_pos != std::string::npos) {
    header_.erase(del_pos, to_remove.size());
  }

  SetTag(kConnectionTypeTag, kConnectionValueClose);
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

  size_t tag_pos = header_.find(tag);
  if (tag_pos == std::string::npos) {
    return "";
  }
  size_t next_endline_pos = header_.find(kLineEnd, tag_pos);
  size_t pos = tag_pos + tag.size() + 1;
  return header_.substr(pos, next_endline_pos - pos);
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

std::string HttpParser::GetHost() const {
  return host_;
}

std::string HttpParser::GetPort() const {
  return port_;
}

std::string HttpParser::GetRequest() const {
  std::string req = "";
  if (length_ == std::string::npos) {
    req = std::move(header_);
  } else {
    req = std::move(header_);
    req += request_.substr(0, length_);
  }
  return req;
}

}
