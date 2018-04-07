#ifndef SOCKETS_EXTERNAL_SERVER_SOCKET_H_
#define SOCKETS_EXTERNAL_SERVER_SOCKET_H_

#include <memory>
#include <string>

#include "epoll.h"
#include "epoll_record.h"
#include "http_parser.h"

namespace sockets {

class ClientSocket;

class ExternalServerSocket : public epoll::EpollRecord {
 public:
  ExternalServerSocket(int fd,
                       std::shared_ptr<epoll::Epoll>,
                       ClientSocket* parent_ptr);
  ~ExternalServerSocket() override;

  void OnIn() override;
  void OnOut() override;
  void OnTimeExpired() override;
  void OnError() override;

  void Disconnect();
  void ReceiveMessageFromParent(const std::string& message);

 private:
  ClientSocket* const parent_ptr_;
  net_utils::HttpParser parser_;
};

}  // namespace sockets

#endif  // SOCKETS_EXTERNAL_SERVER_SOCKET_H_
