#include "epoll.h"

#include <iostream>

#include <sys/epoll.h>

#include "advanced_time.h"
#include "epoll_record.h"
#include "logger.h"
#include "net_utils.h"
#include "signal_handler.h"
#include "terminal_error.h"

namespace epoll {

namespace {

const uint64_t kMaximalEpollWaitTimeMs = 36 * 60 * 1000;

}  // namespace

using base::AdvancedTime;
using base::FileDescriptor;

Epoll::Epoll() : FileDescriptor(net_utils::CreateEpollFD()) {
  if (!base::FileDescriptor::IsValid()) {
    LOGE << "TERMINATION_ERROR: Can't create epoll";
    throw TerminalError();
  }
  LOGI << "Epoll was created; fd: " << GetFD();
}

Epoll::~Epoll() {
  LOGI << "Epoll was destroyed; fd: " << GetFD();
}

void Epoll::Process() {
  FLOGI << "***********************************";
  FLOGI << "Start epoll's processing";
  deleted_records_.clear();

  if (timer_container_.IsEmpty()) {
    LOGE << "TERMINATION_ERROR: Timer container is empty!";
    throw TerminalError();
    return;
  }

  AdvancedTime current_time = AdvancedTime::Now();
  while (!timer_container_.IsEmpty()) {
    auto it = timer_container_.GetNext();
    if (it.GetExpirationTime() < current_time) {
      EpollRecord* record_ptr = it.GetValue();
      LOGW << "Epoll Record: " << record_ptr->GetFD()
            << " was expired";
      record_ptr->OnTimeExpired();
    } else {
      break;
    }
  }

  if (timer_container_.IsEmpty()) {
    LOGE << "TERMINATION_ERROR: Timer container is empty after time expiration!";
    throw TerminalError();
    return;
  }

  AdvancedTime waiting_time =
    timer_container_.GetNext().GetExpirationTime() - current_time;

  uint64_t wtime = waiting_time.GetMilliseconds();
  int current_timeout = -1;
  if (wtime >= kMaximalEpollWaitTimeMs) {
    FLOGI << "Wait any time!";
  } else {
    FLOGI << "Wait for " << waiting_time.GetMilliseconds() << " milliseconds";
    current_timeout = waiting_time.GetMilliseconds();
  }

  int ready = ::epoll_wait(GetFD(), events_,
                           kEpollEventsNumber, current_timeout);

  FLOGI << "Ready " << ready << " records";
  for (int i = 0; i < ready; ++i) {
    epoll_event& event = events_[i];
    EpollRecord* record_ptr = static_cast<EpollRecord*>(event.data.ptr);
    if (deleted_records_.find(record_ptr) != deleted_records_.end()) {
      continue;
    }

    if (!record_ptr->ResetDeadline()) {
      record_ptr->OnError();
      continue;
    }

    if (event.events & EPOLLIN) {
      FLOGI << "OnIn record: " << record_ptr->GetFD();
      record_ptr->OnIn();
      continue;
    }
    if (event.events & EPOLLOUT) {
      FLOGI << "OnOut record: " << record_ptr->GetFD();
      record_ptr->OnOut();
      continue;
    }
    if ((event.events & EPOLLERR) || (event.events & EPOLLHUP)) {
      LOGI << "Epoll error or hup for record: " << record_ptr->GetFD();
      record_ptr->OnError();
      continue;
    }

    LOGE << "Uncatched event: " << event.events
         << " for record: " << record_ptr->GetFD();
    record_ptr->OnError();
  }
  FLOGI << "End epoll's processing";
  FLOGI << "###################################";
}

bool Epoll::SubscribeRecord(EpollRecord* record_ptr) {
  epoll_event ev;
  ev.events = record_ptr->GetFlags();
  ev.data.ptr = static_cast<void*>(record_ptr);

  if (::epoll_ctl(GetFD(), EPOLL_CTL_ADD, record_ptr->GetFD(), &ev) < 0) {
    LOGE << "Error to subscribe record to epoll. fd: " << record_ptr->GetFD();
    return false;
  }

  in_container_[record_ptr] = timer_container_.Insert(
                                record_ptr,
                                record_ptr->GetTimeout(),
                                record_ptr->GetExpirationTime());
  FLOGI << "Record: " << record_ptr->GetFD() << " was subscribed";
  return true;
}

bool Epoll::UpdateRecord(EpollRecord* record_ptr) {
  epoll_event ev;
  ev.events = record_ptr->GetFlags();
  ev.data.ptr = static_cast<void*>(record_ptr);

  if (::epoll_ctl(GetFD(), EPOLL_CTL_MOD, record_ptr->GetFD(), &ev) < 0) {
    LOGE << "Error to modify record in epoll. fd: " << record_ptr->GetFD();
    return false;
  }

  in_container_[record_ptr] = timer_container_.Update(
                                                  in_container_[record_ptr]);
  FLOGI << "Record: " << record_ptr->GetFD() << " was updated";
  return true;
}

bool Epoll::UnsubscribeRecord(EpollRecord* record_ptr) {
  timer_container_.Erase(in_container_[record_ptr]);
  in_container_.erase(record_ptr);
  if (::epoll_ctl(GetFD(), EPOLL_CTL_DEL, record_ptr->GetFD(), NULL) < 0) {
    LOGE << "Error to delete record from epoll. fd: " << record_ptr->GetFD();
    return false;
  }
  FLOGI << "Record: " << record_ptr->GetFD() << " was unsubscribe";
  return true;
}

}  // namespace epoll
