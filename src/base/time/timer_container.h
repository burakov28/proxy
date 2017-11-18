#ifndef BASE_TIME_TIMER_CONTAINER_H_
#define BASE_TIME_TIMER_CONTAINER_H_

#include <list>
#include <map>
#include <memory>
#include <set>

#include "advanced_time.h"
#include "logger.h"
#include "terminal_error.h"

namespace base {

template <typename T>
class TimerContainer {
 private:
  typedef std::list<std::pair<AdvancedTime, T>> List;
  typedef std::shared_ptr<List> ListPtr;
  typedef std::pair<ListPtr, AdvancedTime> ListPtrTimeout;
  typedef std::set<ListPtrTimeout, bool(*)(const ListPtrTimeout&,
                                           const ListPtrTimeout&)> SetForLists;
  typedef std::map<AdvancedTime, typename SetForLists::iterator>
                                                            SetItByTimeout;
 public:
  class Iterator {
   public:
    Iterator() = default;
    Iterator(const AdvancedTime& timeout,
             const typename List::iterator& list_it) :
                timeout_(timeout),
                list_it_(list_it) {}
    Iterator(const Iterator& other) = default;

    ~Iterator() = default;

    AdvancedTime GetTimeout() const {
      return timeout_;
    }

    T GetValue() const {
      return list_it_->second;
    }

    AdvancedTime GetExpirationTime() const {
      return list_it_->first;
    }

   private:
    typename List::iterator GetListIterator() const {
      return list_it_;
    }

    friend class TimerContainer;

    AdvancedTime timeout_;
    typename List::iterator list_it_;
  };

  TimerContainer() : sorted_lists_(&ListsComparator) {}
  ~TimerContainer() {}

  Iterator Insert(const T& value, const AdvancedTime& timeout,
                                  const AdvancedTime& expiration_time) {
    ListPtrTimeout cur;
    typename List::iterator list_it;
    auto it = iterators_by_timeouts_.find(timeout);
    if (it == iterators_by_timeouts_.end()) {
      ListPtr list_ptr = std::make_shared<List>();
      list_it = list_ptr->insert(list_ptr->end(), {expiration_time, value});
      cur = {list_ptr, timeout};
    } else {
      cur = *(it->second);
      sorted_lists_.erase(it->second);
      iterators_by_timeouts_.erase(it);
      list_it = cur.first->insert(cur.first->end(), {expiration_time, value});
    }

    iterators_by_timeouts_[timeout] = sorted_lists_.insert(cur).first;
    return Iterator(timeout, list_it);
  }

  void Erase(const Iterator& it) {
    AdvancedTime timeout = it.GetTimeout();
    auto list_it = it.GetListIterator();
    auto sorted_lists_it = iterators_by_timeouts_[timeout];
    ListPtrTimeout cur = *(sorted_lists_it);
    iterators_by_timeouts_.erase(timeout);
    sorted_lists_.erase(sorted_lists_it);
    cur.first->erase(list_it);
    if (cur.first->empty()) {
      return;
    }
    iterators_by_timeouts_[timeout] = sorted_lists_.insert(cur).first;
  }

  Iterator Update(const Iterator& it, AdvancedTime new_start_time =
                                                        AdvancedTime::Now()) {
    T value = it.GetValue();
    AdvancedTime timeout = it.GetTimeout();
    Erase(it);
    return Insert(value, timeout, new_start_time + timeout);
  }

  Iterator GetNext() const {
    if (IsEmpty()) {
      LOGE << "Timer Container is empty. Cannot retrieve next element";
      throw TerminalError();
    }
    auto it = sorted_lists_.begin();
    return Iterator(it->second, it->first->begin());
  }

  bool IsEmpty() const {
    return iterators_by_timeouts_.empty();
  }

 private:
  static bool ListsComparator(const ListPtrTimeout& lp1,
                              const ListPtrTimeout& lp2) {
    if (lp1.first->empty() || lp2.first->empty()) {
      LOGE << "There is empty list it Timer Container";
      throw TerminalError();
    }
    if (lp1.first->front().first < lp2.first->front().first) {
      return true;
    }
    if (lp1.first->front().first > lp2.first->front().first) {
      return false;
    }
    return lp1.first < lp2.first;
  }

  SetForLists sorted_lists_;
  SetItByTimeout iterators_by_timeouts_;
};

}  // namespace base

#endif  // BASE_TIME_TIMER_CONTAINER_H_
