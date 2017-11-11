#ifndef SOCKETS_SERVER_SOCKET_H_
#define SOCKETS_SERVER_SOCKET_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <netinet/in.h>

#include "epoll.h"
#include "epoll_notifier.h"
#include "epoll_record.h"
#include "id_generator.h"
#include "thread_pool.h"

namespace sockets {

class ClientSocket;

class ServerSocket : public epoll::EpollRecord {
 public:
  ServerSocket(std::shared_ptr<epoll::Epoll> epoll_ptr,
               size_t number_of_threads,
               uint16_t port,
               uint32_t s_addr = INADDR_ANY);

  ~ServerSocket() override;

  void OnIn() override;
  void OnOut() override;
  void OnTimeExpired() override;
  void OnError() override;

  void KillClient(uint64_t id);

  void AddExternalServerToQueue(int external_server_socket_fd,
                                uint64_t client_id);
  void ProcessQueue();

  base::ThreadPool* GetThreadPoolPtr() {
    return &thread_pool_;
  }

 private:
  bool AddClient(int client_fd);

  std::unordered_map<uint64_t, std::shared_ptr<ClientSocket>> clients_;
  base::IdGenerator generator_;

  std::vector<std::pair<int, uint64_t>> queue_;
  std::mutex queue_locker_;
  std::unique_ptr<epoll::EpollNotifier> notifier_;
  base::ThreadPool thread_pool_;
};

}  // namespace sockets

#endif  // SOCKETS_SERVER_SOCKET_H_
