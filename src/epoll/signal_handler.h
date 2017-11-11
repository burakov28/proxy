#ifndef EPOLL_SIGNAL_HANDLER_H_
#define EPOLL_SIGNAL_HANDLER_H_

#include <memory>

#include "epoll_record.h"

namespace epoll {

class Epoll;

class SignalHandler : public EpollRecord {
 public:
  SignalHandler(std::shared_ptr<Epoll> epoll_ptr);
  ~SignalHandler() override;

  void OnIn() override;
  void OnOut() override;
  void OnTimeExpired() override;
  void OnError() override;

  void Disconnect();
};

}  // namespace epoll

#endif  // EPOLL_SIGNAL_HANDLER_H_
