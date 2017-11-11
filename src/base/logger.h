#ifndef BASE_LOGGER_H_
#define BASE_LOGGER_H_

#include <errno.h>
#include <string.h>

#include <sstream>

#define LOG(severity, appendix) ConsoleLogger(__FILE__, __LINE__, severity, appendix)

#define LOGI LOG(LogSeverity::INFO, "")
#define LOGW LOG(LogSeverity::WARNING, "")
#define LOGE LOG(LogSeverity::ERROR, GetErrorMessage(errno))

#define FLOG(severity, appendix) FileLogger(__FILE__, __LINE__, severity, appendix)

#define FLOGI FLOG(LogSeverity::INFO, "")
#define FLOGW FLOG(LogSeverity::WARNING, "")
#define FLOGE FLOG(LogSeverity::ERROR, GetErrorMessage(errno))


std::string GetErrorMessage(int error_number);

enum LogSeverity {
  INFO,
  WARNING,
  ERROR
};

class LogMessage {
 public:
  LogMessage(const char* filename, int line, LogSeverity severity,
             const std::string& appendix, void (*flusher)(const std::string&));
  ~LogMessage();

  template <typename T>
  LogMessage& operator<<(const T& value) {
    message_ << value;
    return *this;
  }

 private:
  std::string appendix_;
  std::stringstream message_;
  void (*flusher_)(const std::string&);
};

void InitFileLogger(const std::string& file);

class FileLogger : public LogMessage {
 public:
  FileLogger(const char* filename, int line, LogSeverity severity,
             const std::string& appendix);

  static void FlushMessage(const std::string&);
};

class ConsoleLogger : public LogMessage {
 public:
  ConsoleLogger(const char* filename, int line, LogSeverity severity,
                const std::string& appendix);

  static void FlushMessage(const std::string&);
};

#endif  // BASE_LOGGER_H_
