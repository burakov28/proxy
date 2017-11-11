#include "advanced_time.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <stdint.h>
#include <string>

#include "logger.h"

namespace base {

namespace {

bool IsOverflowAfterMultiplication(uint64_t val, uint64_t mult) {
  uint64_t max_val = std::numeric_limits<uint64_t>::max();
  return (max_val / mult < val || val * mult == max_val);
}

AdvancedTime InfinityIfOverflowAfterMultiplication(uint64_t val,
                                                   uint64_t mult) {
  if (IsOverflowAfterMultiplication(val, mult)) {
    return AdvancedTime::Infinity();
  }
  return AdvancedTime::FromMilliseconds(val * mult);
}

}  // namespace

// static
AdvancedTime AdvancedTime::Now() {
  auto time_point = std::chrono::high_resolution_clock::now();
  auto dur = time_point.time_since_epoch();
  return FromMilliseconds (std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
}

// static
AdvancedTime AdvancedTime::FromMilliseconds(uint64_t milliseconds) {
  return AdvancedTime(milliseconds);
}

// static
AdvancedTime AdvancedTime::FromSeconds(uint64_t seconds) {
  return InfinityIfOverflowAfterMultiplication(seconds, 1000);
}

// static
AdvancedTime AdvancedTime::Infinity() {
  return AdvancedTime(std::numeric_limits<uint64_t>::max());
}

uint64_t AdvancedTime::GetMilliseconds() const {
  return time_ms_;
}

uint64_t AdvancedTime::GetSeconds() const {
  return time_ms_ / 1000;
}

std::string AdvancedTime::GetDate() const {
  std::chrono::time_point<std::chrono::system_clock> tp(
      std::chrono::duration<uint64_t, std::nano>(time_ms_ * 1000000));

  std::time_t now = std::chrono::system_clock::to_time_t(tp);
  std::stringstream str_stream;
  str_stream << std::put_time(std::localtime(&now), "%F %T");
  str_stream << "." << time_ms_ % 1000;
  return str_stream.str();
}

AdvancedTime::AdvancedTime(uint64_t milliseconds) : time_ms_(milliseconds) {}

bool AdvancedTime::IsFinite() const {
  return time_ms_ != std::numeric_limits<uint64_t>::max();
}

AdvancedTime& AdvancedTime::operator+=(const AdvancedTime& other) {
  if (std::numeric_limits<uint64_t>::max() - time_ms_ <
      other.time_ms_) {
    *this = Infinity();
    return *this;
  }
  time_ms_ += other.time_ms_;
  return *this;
}

AdvancedTime& AdvancedTime::operator-=(const AdvancedTime& other) {
  if (time_ms_ < other.time_ms_) {
    FLOGE << "Try to substract great time from a small one";
    throw std::out_of_range("Try to substract great time from a small one");
  }
  time_ms_ -= other.time_ms_;
  return *this;
}

AdvancedTime AdvancedTime::operator+(const AdvancedTime& other) const {
  AdvancedTime tmp(*this);
  return tmp += other;
}

AdvancedTime AdvancedTime::operator-(const AdvancedTime& other) const {
  AdvancedTime tmp(*this);
  return tmp -= other;
}

bool AdvancedTime::operator==(const AdvancedTime& other) const {
  return time_ms_ == other.time_ms_;
}

bool AdvancedTime::operator<(const AdvancedTime& other) const {
  return time_ms_ < other.time_ms_;
}

bool AdvancedTime::operator<=(const AdvancedTime& other) const {
  return time_ms_ <= other.time_ms_;
}

bool AdvancedTime::operator>(const AdvancedTime& other) const  {
  return time_ms_ > other.time_ms_;
}

bool AdvancedTime::operator>=(const AdvancedTime& other)const {
  return time_ms_ >= other.time_ms_;
}

}  // namespace base
