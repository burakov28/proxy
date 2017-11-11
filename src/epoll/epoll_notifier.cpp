#include "epoll_notifier.h"

#include <functional>

#include <sys/timerfd.h>

#include "advanced_time.h"
#include "logger.h"
#include "terminal_error.h"

namespace epoll {

const size_t kAttemptsToCreateEpollNotifier = 5;

int CreateTimerFD() {
  for (size_t i = 0; i < kAttemptsToCreateEpollNotifier; ++i) {
    int timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd < 0) {
      LOGE << "Error to create EpollNotifier fd; Attempt: " << i + 1;
      continue;
    }
    return timer_fd;
  }
  LOGE << "Error to create EpollNotifier";
  throw TerminalError();
}

EpollNotifier::EpollNotifier(std::shared_ptr<Epoll> epoll_ptr,
                             const Func& on_in_handler) :
      EpollRecord(CreateTimerFD(),
                  epoll_ptr,
                  EpollRecord::IN,
                  base::AdvancedTime::Infinity()),
      on_in_handler_(on_in_handler){
  LOGI << "EpollNotifier was created; fd: " << GetFD();
}

EpollNotifier::~EpollNotifier() {
  LOGI << "EpollNotifier was destroyed; fd: " << GetFD();
}

void EpollNotifier::Notify() {
  ::itimerspec exp;
  exp.it_interval.tv_sec = 0;
  exp.it_interval.tv_nsec = 0;
  exp.it_value.tv_sec = 0;
  exp.it_value.tv_nsec = 100;
  if (::timerfd_settime(GetFD(), 0, &exp, NULL) < 0) {
    LOGE << "Error to set time for EpollNotifier; fd: " << GetFD();
    throw TerminalError();
  }
  FLOGI << "Time for EpollNotifier was set; fd: " << GetFD();
}

void EpollNotifier::OnIn() {
  std::string times = IOFileDescriptor::Read();
  uint64_t* times_ptr = (uint64_t*)((void*) times.c_str());
  FLOGI << "EpollNotifier was triggered " << *times_ptr << " times; fd: "
        << GetFD();
  on_in_handler_();
}

void EpollNotifier::OnOut() {
  LOGE << "TERMINATION_ERROR: "
       << "EpollNotifier was triggered for out; fd: " << GetFD();
  throw TerminalError();
}

void EpollNotifier::OnTimeExpired() {
  LOGE << "TERMINATION_ERROR: "
       << "EpollNotifier was triggered for time expired; fd: " << GetFD();
  throw TerminalError();
}

void EpollNotifier::OnError() {
  LOGE << "TERMINATION_ERROR: "
       << "EpollNotifier was triggered for error; fd: " << GetFD();
  throw TerminalError();
}

}  // namespace epoll
