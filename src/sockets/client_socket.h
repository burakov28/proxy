#ifndef SOCKETS_CLIENT_SOCKET_H_
#define SOCKETS_CLIENT_SOCKET_H_

#include <memory>

#include "advanced_time.h"
#include "epoll.h"
#include "epoll_record.h"
#include "external_server_socket.h"
#include "http_parser.h"

namespace sockets {

class ServerSocket;

class ClientSocket : public epoll::EpollRecord {
 public:
  ClientSocket(int fd, std::shared_ptr<epoll::Epoll> epoll_ptr,
               ServerSocket* server_ptr, uint64_t id);

  ~ClientSocket() override;

  void OnIn() override;
  void OnOut() override;
  void OnTimeExpired() override;
  void OnError() override;

  void Disconnect();
  void DisconnectOnSend();

  static void CreateExternalServer(std::string host,
                                   std::string port,
                                   ServerSocket* server_ptr,
                                   uint64_t id);
  void SetExternalServer(int external_server_socket_fd);
  void KillExternalServer();
  void ReceiveMessageFromExternalServer(const std::string& data);

 private:
  ServerSocket* const server_ptr_;
  net_utils::HttpParser parser_;
  std::unique_ptr<ExternalServerSocket> external_server_ptr_;
  uint64_t id_;
  bool is_disconnect_on_send_;
};

}  // namespace sockets

#endif
