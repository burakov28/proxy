#ifndef BASE_TIME_ADVANCED_TIME_H_
#define BASE_TIME_ADVANCED_TIME_H_

#include <stdint.h>
#include <string>

namespace base {

class AdvancedTime {
 public:
  AdvancedTime() = default;
  AdvancedTime(const AdvancedTime& other) = default;
  AdvancedTime(AdvancedTime&& other) = default;

  AdvancedTime& operator=(const AdvancedTime& other) = default;

  static AdvancedTime Now();
  static AdvancedTime FromSeconds(uint64_t seconds);
  static AdvancedTime FromMilliseconds(uint64_t milliseconds);
  static AdvancedTime Infinity();

  uint64_t GetSeconds() const;
  uint64_t GetMilliseconds() const;
  std::string GetDate() const;

  bool IsFinite() const;

  AdvancedTime& operator+=(const AdvancedTime& other);
  AdvancedTime& operator-=(const AdvancedTime& other);

  AdvancedTime operator+(const AdvancedTime& other) const;
  AdvancedTime operator-(const AdvancedTime& other) const;

  bool operator==(const AdvancedTime& other) const;

  bool operator<(const AdvancedTime& other) const;
  bool operator<=(const AdvancedTime& other) const;
  bool operator>(const AdvancedTime& other) const;
  bool operator>=(const AdvancedTime& other) const;

 private:
  AdvancedTime(uint64_t time_ms);

  uint64_t time_ms_;
};

}  // namespace base


#endif  // BASE_TIME_ADVANCED_TIME_H_
