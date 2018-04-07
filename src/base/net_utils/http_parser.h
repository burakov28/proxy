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

  HttpParser(bool is_request = true);
  ~HttpParser();

  AppendResult Append(const std::string& message);

  std::string GetRequest() const;
  std::string GetHost() const;
  std::string GetPort() const;

  void ModifyHeader();

 private:
  std::string GetTagValue(const std::string& tag) const;

  void ParseLength();
  void ParseHostAndPort();

  bool ParseHeader();

  void SetTag(const std::string&, const std::string&);
  void InsertTag(const std::string&, const std::string&);
  void ReplaceTag(const std::string&, const std::string&, size_t);

  std::string header_;
  std::string host_;
  std::string port_;
  std::string request_;
  size_t length_;
  size_t end_header_pos_;
  bool is_request_;
};

}

#endif  // BASE_NET_UTILS_HTTP_PARSER_H_
