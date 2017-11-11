#ifndef EPOLL_EPOLL_RECORD_H_
#define EPOLL_EPOLL_RECORD_H_

#include <memory>

#include "advanced_time.h"
#include "epoll.h"
#include "file_descriptor.h"

namespace epoll {

class EpollRecord : public base::IOFileDescriptor {
 public:
  enum Flags {
    IN = EPOLLIN,
    OUT = EPOLLOUT,
  };

  EpollRecord(int fd_,
              std::shared_ptr<Epoll> epoll_ptr,
              uint32_t flags,
              base::AdvancedTime timeout,
              base::AdvancedTime delay =
                  base::AdvancedTime::FromMilliseconds(0));

  virtual ~EpollRecord();

  uint32_t GetFlags() const;
  base::AdvancedTime GetTimeout() const;
  base::AdvancedTime GetExpirationTime() const;

  bool ResetDeadline();
  bool SetFlags(uint32_t new_flags);

  virtual void OnIn() = 0;
  virtual void OnOut() = 0;
  virtual void OnTimeExpired() = 0;
  virtual void OnError() = 0;

  std::shared_ptr<Epoll> GetEpollPtr() const;

 private:
  base::AdvancedTime timeout_;
  base::AdvancedTime expiration_time_;
  uint32_t flags_;
  std::shared_ptr<Epoll> epoll_ptr_;
};

}  // namespace epoll

#endif  // BASE_EPOLL_EPOLL_RECORD_H_
