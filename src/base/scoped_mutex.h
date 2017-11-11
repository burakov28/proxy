#ifndef BASE_SCOPED_MUTEX_H_
#define BASE_SCOPED_MUTEX_H_

#include <mutex>

class ScopedMutex {
 public:
  ScopedMutex(std::mutex* locker) : locker_(locker) {
    locker_->lock();
  }

  ~ScopedMutex() {
    if (locker_ == nullptr)
      return;
    locker_->unlock();
  }

  void Release() {
    locker_->unlock();
    locker_ = nullptr;
  }

 private:
  std::mutex* locker_;
};

#endif  // BASE_SCOPED_MUTEX_H_
