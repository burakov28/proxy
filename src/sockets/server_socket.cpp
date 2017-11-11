#include "server_socket.h"

#include "client_socket.h"
#include "id_generator.h"
#include "logger.h"
#include "net_utils.h"
#include "scoped_mutex.h"
#include "terminal_error.h"
#include "thread_pool.h"

namespace sockets {

using epoll::Epoll;
using epoll::EpollRecord;
using base::AdvancedTime;

ServerSocket::ServerSocket(std::shared_ptr<Epoll> epoll_ptr,
                           size_t number_of_threads,
                           uint16_t port,
                           uint32_t s_addr) :
    EpollRecord(net_utils::CreateServerSocket(port, s_addr),
                epoll_ptr,
                EpollRecord::IN,
                AdvancedTime::Infinity()),
    thread_pool_(number_of_threads) {
  notifier_.reset(new epoll::EpollNotifier(epoll_ptr, [this](){
    ProcessQueue();
  }));
  LOGI << "Server Socket was created; fd: " << GetFD()
       << "; for port: " << port;
}

ServerSocket::~ServerSocket() {
  LOGI << "Server Socket is destroying; fd: " << GetFD();
}

void ServerSocket::OnIn() {
  int client_fd = net_utils::AcceptNonblocking(GetFD());
  if (client_fd < 0) {
    LOGE << "Can't accept client. Skip it. " << client_fd;
    return;
  }
  if (!AddClient(client_fd)) {
    LOGE << "Can't add client: " << client_fd << ". Skip it.";
    return;
  }
  FLOGI << "Accepted client with fd: " << client_fd;
}

void ServerSocket::OnOut() {
  LOGE << "TERMINATION_ERROR Server socket has been triggered for OUT.";
  throw TerminalError();
}

void ServerSocket::OnTimeExpired() {
  LOGE << "TERMINATION_ERROR Server socket has been expired.";
  throw TerminalError();
}

void ServerSocket::OnError() {
  LOGE << "TERMINATION_ERROR Server socket was broken!";
  throw TerminalError();
}

bool ServerSocket::AddClient(int client_fd) {
  uint64_t new_id = generator_.GetNext();
  std::shared_ptr<ClientSocket> client_ptr =
      std::make_shared<ClientSocket>(client_fd,
                                     EpollRecord::GetEpollPtr(),
                                     this,
                                     new_id);
  clients_[new_id] = client_ptr;
  return true;
}

void ServerSocket::KillClient(uint64_t id) {
  clients_.erase(id);
}

void ServerSocket::AddExternalServerToQueue(int external_server_socket_fd,
                                            uint64_t client_id) {
  ScopedMutex scoped_mutex(&queue_locker_);
  queue_.push_back({external_server_socket_fd, client_id});
  notifier_->Notify();
}

void ServerSocket::ProcessQueue() {
  ScopedMutex scoped_mutex(&queue_locker_);
  for (auto p : queue_) {
    int fd = p.first;
    int client_id = p.second;
    if (clients_.find(client_id) != clients_.end()) {
      clients_[client_id]->SetExternalServer(fd);
    }
  }
  queue_.clear();
}

}  // namespace sockets
