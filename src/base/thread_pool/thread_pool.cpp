#include "thread_pool.h"

#include <iostream>

#include "logger.h"
#include "terminal_error.h"

namespace base {

ThreadPool::ThreadPool(size_t number_of_threads)  :
      number_of_threads_(number_of_threads),
      is_stopped_(false) {
  if (number_of_threads_ > kMaxNumberOfThreads) {
    LOGE << "Too many threads for thread pool";
    throw TerminalError();
  }
  threads_ = std::unique_ptr<std::thread[]>(
      new std::thread[number_of_threads_]);

  std::unique_lock<std::mutex> lock(locker_);
  for (size_t i = 0; i < number_of_threads_; ++i) {
    threads_[i] = std::thread(ThreadTask(this, i + 1));
  }
  LOGI << "Thread pool was created";
}

ThreadPool::~ThreadPool() {
  LOGI << "Shutdowning thread pool";
  std::unique_lock<std::mutex> lock(locker_);
  is_stopped_.store(true);
  lock.unlock();
  lock.release();
  monitor_.notify_all();

  LOGI << "Waiting for threads ends";
  for (size_t i = 0; i < number_of_threads_; ++i) {
    threads_[i].join();
    LOGI << i + 1 << "th thread stopped";
  }
  LOGI << "All threads are stopped";
}

void ThreadPool::PostTask(const Func& task) {
  FLOGI << "Posting Task...";
  std::unique_lock<std::mutex> lock(locker_);
  queue_.push(task);
  FLOGI << "Task was posted";
  monitor_.notify_one();
}

ThreadPool::ThreadTask::ThreadTask(ThreadPool* pool_ptr, uint32_t thread_num) :
    pool_ptr_(pool_ptr), thread_num_(thread_num) {}

ThreadPool::ThreadTask::~ThreadTask() {}

void ThreadPool::ThreadTask::operator()() {
  std::stringstream name_stream;
  name_stream << "ThreadPoolLog_" << thread_num_ << ".log";
  InitFileLogger(name_stream.str());

  FLOGI << "Start Run Loop";
  while (true) {
    std::unique_lock<std::mutex> lock(pool_ptr_->locker_);
    while (pool_ptr_->queue_.empty() && !pool_ptr_->is_stopped_.load()) {
      pool_ptr_->monitor_.wait(lock);
    }

    if (pool_ptr_->is_stopped_.load()) {
      break;
    }

    if (pool_ptr_->queue_.empty()) {
      FLOGE << "FALSE WAKEUP!!!";
      continue;
    }
    Func task = pool_ptr_->queue_.front();
    pool_ptr_->queue_.pop();
    lock.unlock();
    lock.release();

    FLOGI << "*********************************************";
    FLOGI << "Process task...";
    task();
    FLOGI << "Task was processed";
    FLOGI << "#############################################\n";
  }

  FLOGI << "Stop Run Loop";
}

}  // namespace base
