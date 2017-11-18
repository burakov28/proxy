#include "epoll_record.h"

#include "logger.h"

namespace epoll {

using base::AdvancedTime;
using base::IOFileDescriptor;

EpollRecord::EpollRecord(int fd,
                         std::shared_ptr<Epoll> epoll_ptr,
                         uint32_t flags,
                         AdvancedTime timeout,
                         AdvancedTime delay) :
    IOFileDescriptor(fd),
    timeout_(timeout),
    expiration_time_(AdvancedTime::Now() + delay + timeout),
    flags_(flags),
    epoll_ptr_(epoll_ptr) {
  if (!epoll_ptr_->SubscribeRecord(this)) {
    LOGE << "Error to subscribe record: " << fd;
    throw std::runtime_error("Can't create epoll record");
  }
  FLOGI << "Record was created; fd: " << GetFD();
}

EpollRecord::~EpollRecord() {
  if (!epoll_ptr_->UnsubscribeRecord(this)) {
    LOGE << "Error to unsubscribe record: " << GetFD();
  }
  FLOGI << "Record was destroyed; with fd: " << GetFD();
}

uint32_t EpollRecord::GetFlags() const {
  return flags_;
}

AdvancedTime EpollRecord::GetTimeout() const {
  return timeout_;
}

AdvancedTime EpollRecord::GetExpirationTime() const {
  return expiration_time_;
}

bool EpollRecord::SetFlags(uint32_t new_flags) {
  flags_ = new_flags;
  bool ret = epoll_ptr_->UpdateRecord(this);
  if (!ret) {
    LOGE << "Error to set flags for record: " << GetFD();
  }
  return ret;
}

bool EpollRecord::AddFlag(uint32_t flag) {
  uint32_t flags = GetFlags();
  if (flags & flag)
    return true;
  return SetFlags(flags | flag);
}

bool EpollRecord::RemoveFlag(uint32_t flag) {
  uint32_t flags = GetFlags();
  flags = (flags & (~flag));
  return SetFlags(flags);
}

bool EpollRecord::ResetDeadline() {
  expiration_time_ = AdvancedTime::Now() + timeout_;
  bool ret = epoll_ptr_->UpdateRecord(this);
  if (!ret) {
    LOGE << "Error to reset deadline for record: " << GetFD();
  }
  return ret;
}

std::shared_ptr<Epoll> EpollRecord::GetEpollPtr() const {
  return epoll_ptr_;
}

}  // namespace epoll
