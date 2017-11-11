#ifndef EPOLL_EPOLL_NOTIFIER_H_
#define EPOLL_EPOLL_NOTIFIER_H_

#include <functional>

#include "epoll_record.h"
#include "logger.h"
#include "terminal_error.h"

namespace epoll {

class EpollNotifier : public EpollRecord {
 private:
  typedef std::function<void(void)> Func;

 public:
  EpollNotifier(std::shared_ptr<Epoll> epoll_ptr,
                const Func& on_in_handler = [](){LOGE << "Uninitialized timer";
                                                 throw TerminalError(); });
  ~EpollNotifier() override;

  void OnIn() override;
  void OnOut() override;
  void OnTimeExpired() override;
  void OnError() override;

  void Notify();

 private:
  Func on_in_handler_;
};

}

#endif  // EPOLL_EPOLL_NOTIFIER_H_
