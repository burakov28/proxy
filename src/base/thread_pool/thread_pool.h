#ifndef BASE_THREAD_POOL_THREAD_POOL_H_
#define BASE_THREAD_POOL_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace base {

const size_t kDefaultNumberOfThreads = 4;
const size_t kMaxNumberOfThreads = 1024;

class ThreadPool {
 private:
  typedef std::function<void(void)> Func;

 public:
  ThreadPool(size_t number_of_threads = kDefaultNumberOfThreads);
  ~ThreadPool();

  void PostTask(const Func& task);

 private:
  class ThreadTask {
   public:
    ThreadTask(ThreadPool* pool_ptr, uint32_t thread_num);
    ~ThreadTask();

    void operator()();

   private:
    ThreadPool* pool_ptr_;
    const uint32_t thread_num_;

    friend class ThreadPool;
  };

  std::unique_ptr<std::thread[]> threads_;
  size_t number_of_threads_;
  std::queue<Func> queue_;
  std::atomic<bool> is_stopped_;
  std::mutex locker_;
  std::condition_variable monitor_;
};

}  // namespace base

#endif  // BASE_THREAD_POOL_THREAD_POOL_H_
