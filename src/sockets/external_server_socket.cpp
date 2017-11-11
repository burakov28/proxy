#include "external_server_socket.h"

#include <memory>

#include "advanced_time.h"
#include "client_socket.h"
#include "epoll.h"
#include "epoll_record.h"
#include "logger.h"

namespace sockets {

using base::AdvancedTime;
using base::IOFileDescriptor;
using epoll::Epoll;
using epoll::EpollRecord;

namespace {

const uint64_t kTimeoutForExternalServerIdleMs = 20000;

}  // namespace

ExternalServerSocket::ExternalServerSocket(int fd,
                                           std::shared_ptr<Epoll> epoll_ptr,
                                           ClientSocket* parent_ptr) :
    EpollRecord(fd, epoll_ptr, EpollRecord::IN,
            AdvancedTime::FromMilliseconds(kTimeoutForExternalServerIdleMs)),
    parent_ptr_(parent_ptr) {
  LOGI << "External Server was created; fd: " << GetFD();
}

ExternalServerSocket::~ExternalServerSocket() {
  LOGI << "External Server was destroyed; fd: " << GetFD();
}

void ExternalServerSocket::OnIn() {
  std::string message = IOFileDescriptor::Read();
  if (message == "") {
    LOGI << "External Server read empty message; fd: " << GetFD()
         << "; close connection";
    Disconnect();
    return;
  }
  flagOGI << message;
  parent_ptr_->ReceiveMessageFromExternalServer(message);
  return;
}

void ExternalServerSocket::OnOut() {
  if (IOFileDescriptor::IsEmpty()) {
    LOGE << "Nothing to write. External Server: " << GetFD()
         << "; close connection";
    Disconnect();
    return;
  }
  if (!IOFileDescriptor::Write()) {
    LOGE << "Writing error. External Server: " << GetFD()
         << "; close connection";
    Disconnect();
    return;
  }
  if (IOFileDescriptor::IsEmpty()) {
    uint32_t flags = EpollRecord::GetFlags();
    if (!EpollRecord::SetFlags(flags & (~EpollRecord::OUT))) {
      LOGE << "Error to set OUT flag after empty buffer of ext_server: "
           << GetFD() << "; close connection";
      Disconnect();
    }
  }
}

void ExternalServerSocket::OnTimeExpired() {
  LOGE << "External server was expired; fd: " << GetFD()
       << "; close connection";
  Disconnect();
}

void ExternalServerSocket::OnError() {
  LOGE << "External server error occured; fd: " << GetFD()
       << "; close connection";
  Disconnect();
}

void ExternalServerSocket::Disconnect() {
  parent_ptr_->KillExternalServer();
}

void ExternalServerSocket::ReceiveMessageFromParent(const std::string& data) {
  if (!IOFileDescriptor::Append(data.c_str(), data.size())) {
    LOGE << "Error to receive message from parent; ext_server: " << GetFD()
         << "; close connection";
    Disconnect();
    return;
  }
  uint32_t flags = EpollRecord::GetFlags();
  if (flags & EpollRecord::OUT) {
    return;
  }
  if (!EpollRecord::SetFlags(flags | EpollRecord::OUT)) {
    LOGE << "Error to set OUT flag after receiving message from parent; "
         << "ext_server: " << GetFD() << "; close connection";
    Disconnect();
  }
}

}  // namespace sockets
