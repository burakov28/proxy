#include "client_socket.h"

#include <memory>

#include "advanced_time.h"
#include "epoll.h"
#include "epoll_record.h"
#include "external_server_socket.h"
#include "http_parser.h"
#include "logger.h"
#include "net_utils.h"
#include "server_socket.h"

namespace sockets {

using epoll::Epoll;
using epoll::EpollRecord;
using base::AdvancedTime;
using net_utils::HttpParser;

namespace {

const uint64_t kTimeoutForClientsIdleMs = 60000;
const char kConstPortNumberString[] = "80";

}  // namespace

ClientSocket::ClientSocket(int fd, std::shared_ptr<Epoll> epoll_ptr,
                           ServerSocket* server_ptr, uint64_t id) :
    EpollRecord(fd, epoll_ptr, EpollRecord::IN,
                AdvancedTime::FromMilliseconds(kTimeoutForClientsIdleMs)),
    server_ptr_(server_ptr), id_(id), is_disconnect_on_send_(false) {
  LOGI << "Client Socket was created; fd: " << GetFD();
}

ClientSocket::~ClientSocket() {
  LOGI << "Client Socket is destroying; fd: " << GetFD();
}

void ClientSocket::OnIn() {
  std::string message = IOFileDescriptor::Read();
  if (message.empty()) {
    LOGI << "Empty message from client: " << GetFD() << "; close connection";
    Disconnect();
    return;
  }
  std::string host, port;
  ServerSocket* server_tmp_ptr;
  uint64_t id_tmp;
  switch (parser_.Append(message)) {
    case (HttpParser::AppendResult::ERROR):
      LOGE << "Request too large. Client: " << GetFD() << "; close connection";
      Disconnect();
      return;

    case (HttpParser::AppendResult::READY_REQUEST):
      host = parser_.GetHost();
      port = parser_.GetPort();
      server_tmp_ptr = server_ptr_;
      id_tmp = id_;
      server_ptr_->GetThreadPoolPtr()->PostTask(
          [host, port, server_tmp_ptr, id_tmp]() {
            ClientSocket::CreateExternalServer(host,
                                               port,
                                               server_tmp_ptr,
                                               id_tmp);
          });
      if (!EpollRecord::RemoveFlag(EpollRecord::IN)) {
        LOGE << "Error to remove IN flag. Fd: " << GetFD();
        Disconnect();
        return;
      }
      return;

    case (HttpParser::AppendResult::SUCCESS):
      return;

    default:
      LOGE << "Unsupported Append Result; fd: " << GetFD()
           << "; close connection";
      Disconnect();
      return;
  }
}

// static
void ClientSocket::CreateExternalServer(std::string host,
                                        std::string port,
                                        ServerSocket* server_ptr,
                                        uint64_t id) {
  int external_server_socket_fd =
      net_utils::CreateExternalServerSocket(
            host.c_str(),
            (port != "") ? (port.c_str()) : (kConstPortNumberString));
  server_ptr->AddExternalServerToQueue(external_server_socket_fd, id);
}

void ClientSocket::SetExternalServer(int external_server_socket_fd) {
  if (external_server_socket_fd < 0) {
    LOGE << "Error to create external server; client: " << GetFD();
    KillExternalServer();
    return;
  }
  try {
    external_server_ptr_.reset(
        new ExternalServerSocket(external_server_socket_fd,
                                 GetEpollPtr(),
                                 this));
  } catch (...) {
    KillExternalServer();
    return;
  }

  parser_.ModifyHeader();

  external_server_ptr_->ReceiveMessageFromParent(parser_.GetRequest());
}

void ClientSocket::OnOut() {
  if (IOFileDescriptor::IsEmpty()) {
    LOGE << "Nothing to write. Client: " << GetFD() << "; close connection";
    Disconnect();
    return;
  }
  if (!IOFileDescriptor::Write()) {
    LOGE << "Writing error. Client: " << GetFD() << "; close connection";
    Disconnect();
    return;
  }
  if (IOFileDescriptor::IsEmpty()) {
    if (!EpollRecord::RemoveFlag(EpollRecord::OUT)) {
      LOGE << "Error to remove OUT flag from client after buffer became empty"
           << "; client: " << GetFD() << "; close connection";
      Disconnect();
    }

    if (is_disconnect_on_send_) {
      Disconnect();
    }
  }
}

void ClientSocket::OnTimeExpired() {
  LOGW << "Time expired for client: " << GetFD();
  Disconnect();
}

void ClientSocket::OnError() {
  LOGE << "Error ocurred for client: " << GetFD();
  Disconnect();
}

void ClientSocket::Disconnect() {
  server_ptr_->KillClient(id_);
}

void ClientSocket::DisconnectOnSend() {
  if (IOFileDescriptor::IsEmpty()) {
    Disconnect();
    return;
  }

  is_disconnect_on_send_ = true;
}

void ClientSocket::KillExternalServer() {
  external_server_ptr_.reset(nullptr);

  uint32_t flags = EpollRecord::GetFlags();
  if (!EpollRecord::SetFlags(flags | EpollRecord::IN)) {
    LOGE << "Error to restore IN flag after kill external server; client: "
         << GetFD() << "; close connection";
  }
  DisconnectOnSend();
}

void ClientSocket::ReceiveMessageFromExternalServer(const std::string& data) {
  if (!IOFileDescriptor::Append(data.c_str(), data.size())) {
    LOGE << "Client buffer overflowed; fd: " << GetFD()
         << "; close connection";
    Disconnect();
    return;
  }
  uint32_t flags = EpollRecord::GetFlags();
  if (flags & EpollRecord::OUT) {
    return;
  }
  if (!EpollRecord::AddFlag(EpollRecord::OUT)) {
    LOGE << "Error to change flags after receiving message; client: "
         << GetFD() << "; close connection";
  }
}

}  // namespace sockets
