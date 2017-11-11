#ifndef BASE_TIME_TIMER_CONTAINER_H_
#define BASE_TIME_TIMER_CONTAINER_H_

#include <list>
#include <map>
#include <memory>

#include "advanced_time.h"

namespace base {

template <typename T>
class TimerContainer {
 private:
  typedef std::list<std::pair<AdvancedTime, T>> List;
  typedef std::shared_ptr<List> ListPtr;
  typedef std::multimap<AdvancedTime, ListPtr> MultiMap;
  typedef std::map<AdvancedTime, typename MultiMap::iterator>
      MapTimeoutsToPosInMultiMap;
  typedef std::map<ListPtr, AdvancedTime> MapListsToTimeouts;

 public:
  class Iterator {
   public:
    Iterator() {}

    Iterator(const typename List::iterator& list_it,
      const typename MapTimeoutsToPosInMultiMap::const_iterator& timeout_it) :
                list_it_(list_it), timeout_it_(timeout_it) {}

    Iterator(const Iterator& other) : list_it_(other.list_it_),
                                      timeout_it_(other.timeout_it_) {}

    const AdvancedTime& GetTimeout() const {
      return timeout_it_->first;
    }

    const T& GetValue() const {
      return list_it_->second;
    }

    const AdvancedTime& GetExpirationTime() const {
      return list_it_->first;
    }

   private:
    ListPtr GetListPtr() const {
      return timeout_it_->second->second;
    }

    friend TimerContainer;

    typename List::iterator list_it_;
    typename MapTimeoutsToPosInMultiMap::const_iterator timeout_it_;
  };

  Iterator Insert(const T& value, AdvancedTime timeout,
                  AdvancedTime expiration_time = AdvancedTime::Now()) {
    ListPtr list_ptr;
    typename List::iterator list_it;
    typename MapTimeoutsToPosInMultiMap::iterator timeout_it;

    auto it = timeouts_in_multimap_.find(timeout);
    if (it == timeouts_in_multimap_.end()) {
      ListPtr list_ptr = std::make_shared<List>();
      list_it = list_ptr->insert(list_ptr->end(), {expiration_time, value});
      timeouts_by_lists_[list_ptr] = timeout;
      timeout_it = timeouts_in_multimap_.insert({
          timeout,
          lists_sorted_by_expiration_time_.insert({expiration_time,
                                                   list_ptr})}).first;
    } else {
      list_ptr = it->second->second;
      list_it = list_ptr->insert(list_ptr->end(), {expiration_time, value});
      timeout_it = it;
    }
    return Iterator(list_it, timeout_it);
  }

  void Erase(const Iterator& it) {
    ListPtr list_ptr = it.GetListPtr();
    AdvancedTime timeout = it.GetTimeout();

    if (it.list_it_ != list_ptr->begin()) {
      list_ptr->erase(it.list_it_);
      return;
    }

    list_ptr->erase(it.list_it_);
    lists_sorted_by_expiration_time_.erase(it.timeout_it_->second);
    timeouts_in_multimap_.erase(timeout);
    if (list_ptr->size() == 0) {
      timeouts_by_lists_.erase(list_ptr);
      return;
    }
    timeouts_in_multimap_[timeout] =
        lists_sorted_by_expiration_time_.insert({list_ptr->front().first,
                                                 list_ptr});
  }

  Iterator GetNext() const {
    ListPtr list_ptr = lists_sorted_by_expiration_time_.begin()->second;
    auto list_it = list_ptr->begin();
    AdvancedTime timeout = timeouts_by_lists_.at(list_ptr);
    return Iterator(list_it, timeouts_in_multimap_.find(timeout));
  }

  Iterator Update(const Iterator& it, AdvancedTime new_start_time =
                                                        AdvancedTime::Now()) {
    T value = it.GetValue();
    AdvancedTime timeout = it.GetTimeout();
    Erase(it);
    return Insert(value, timeout, new_start_time + timeout);
  }

  bool IsEmpty() const {
    return lists_sorted_by_expiration_time_.empty();
  }

 private:
  MultiMap lists_sorted_by_expiration_time_;
  MapTimeoutsToPosInMultiMap timeouts_in_multimap_;
  MapListsToTimeouts timeouts_by_lists_;
};

}  // namespace base

#endif  // BASE_TIME_TIMER_CONTAINER_H_
