#include "signal_handler.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/signalfd.h>

#include "advanced_time.h"
#include "epoll.h"
#include "epoll_record.h"
#include "logger.h"
#include "terminal_error.h"

namespace epoll {

namespace {

int GetSignalFD() {
  sigset_t mask;
  if (::sigemptyset(&mask) < 0) {
    LOGE << "Unable to create empty signal mask";
    return -1;
  }
  if (::sigaddset(&mask, SIGTERM) < 0) {
    LOGE << "Unable to add SIGTERM to mask";
    return -1;
  }
  if (::sigaddset(&mask, SIGINT)) {
    LOGE << "Unable to add SIGINT to mask";
    return -1;
  }
  if (::sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    LOGE << "Unable to process signal mask";
    return -1;
  }

  int fd = ::signalfd(-1, &mask, O_NONBLOCK);
  if (fd < 0) {
    LOGE << "Unable to create signal fd";
    return -1;
  }
  return fd;
}

}

SignalHandler::SignalHandler(std::shared_ptr<Epoll> epoll_ptr) :
    EpollRecord(GetSignalFD(),
                epoll_ptr,
                EpollRecord::IN,
                base::AdvancedTime::Infinity()) {
  LOGI << "Signal Handler was created; fd: " << GetFD();
}

SignalHandler::~SignalHandler() {
  LOGI << "Signal handler was destroyed; fd: " << GetFD();
}

void SignalHandler::OnIn() {
  LOGI << "Signal Handler catch termination signal";
  Disconnect();
}

void SignalHandler::OnOut() {
  LOGE << "Signal fd isn't subscribe for out action";
  if (!EpollRecord::SetFlags(EpollRecord::IN)) {
    Disconnect();
  }
}

void SignalHandler::OnTimeExpired() {
  LOGE << "Signal fd time expired";
  Disconnect();
}

void SignalHandler::OnError() {
  LOGE << "Signal fd processor's error";
  Disconnect();
}

void SignalHandler::Disconnect() {
  throw TerminalError();
}

}  // namespace epoll
