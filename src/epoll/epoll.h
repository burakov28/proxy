#ifndef EPOLL_EPOLL_H_
#define EPOLL_EPOLL_H_

#include <sys/epoll.h>

#include <map>
#include <memory>
#include <unordered_set>

#include "file_descriptor.h"
#include "timer_container.h"

namespace epoll {

class EpollRecord;
class SignalHandler;

class Epoll : public base::FileDescriptor {
 public:
  Epoll();
  ~Epoll();

  void Process();

 private:
  bool SubscribeRecord(EpollRecord* record);
  bool UpdateRecord(EpollRecord* record);
  bool UnsubscribeRecord(EpollRecord* record);

  friend class EpollRecord;

  static const size_t kEpollEventsNumber = 1024;

  base::TimerContainer<EpollRecord*> timer_container_;
  std::unordered_set<EpollRecord*> deleted_records_;
  std::map<EpollRecord*,
           base::TimerContainer<EpollRecord*>::Iterator> in_container_;
  epoll_event events_[kEpollEventsNumber];
};

}  // namespace epoll

#endif  // BASE_EPOLL_EPOLL_H_
