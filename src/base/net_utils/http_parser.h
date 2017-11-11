#ifndef BASE_NET_UTILS_HTTP_PARSER_H_
#define BASE_NET_UTILS_HTTP_PARSER_H_

#include <string>

namespace net_utils {

class HttpParser {
 public:
  enum AppendResult {
    SUCCESS,
    READY_REQUEST,
    ERROR
  };

  HttpParser();
  ~HttpParser();

  AppendResult Append(const std::string& message);

  std::string GetNextRequest();
  std::string GetHost() const;
  std::string GetPort() const;

 private:
  void ParseHostAndPort();
  void ParseLength();
  std::string GetTagValue(const std::string& tag) const;
  bool FindNextHeader() const;
  void ResetFields();
  bool ParseHeader();

  std::string header_;
  std::string ready_request_;
  std::string request_;
  size_t end_header_pos_;
  std::string host_;
  std::string port_;
  size_t length_;
};

}

#endif  // BASE_NET_UTILS_HTTP_PARSER_H_
