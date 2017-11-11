#include "logger.h"

#include <cstdio>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "scoped_mutex.h"
#include "advanced_time.h"

namespace {

const char kOldFileSuffix[] = "_old";
const std::string kProjectFolder = "src/";
const uint32_t kMaxTimesBeforeFlush = 20;  // records before flushing;
const uint64_t kMaxLogFileSize = 4 * 1024 * 1024;  // in bytes;

std::string GetPathInProject(const std::string& path) {
  size_t pos = path.rfind(kProjectFolder);
  if (pos == std::string::npos) {
    return path;
  }
  pos += kProjectFolder.size();
  return path.substr(pos, path.size() - pos);
}

std::string LogSeverityToString(LogSeverity severity) {
  switch(severity) {
    case INFO:
      return "INFO";
    case WARNING:
      return "WARNING";
    case ERROR:
      return "ERROR";
    default:
      return "UNKNOWN_SEVERITY";
  }
}

uint64_t GetFileSize(const std::string& file_path) {
  struct stat st;
  if (::stat(file_path.c_str(), &st) < 0) {
    LOGE << "Error to retrieve information about log file: " << file_path;
    return 0;
  }
  uint64_t size = st.st_size;
  return size;
}

std::string GetOldLogFileName(const std::string& file_path) {
  int64_t pos = file_path.size() - 1;
  for (; pos >= 0 && file_path[pos] != '.'; --pos);
  if (pos < 0) {
    return file_path + kOldFileSuffix;
  }
  std::string base_name = file_path.substr(0, pos);
  return base_name + kOldFileSuffix +
            file_path.substr(pos, file_path.size() - pos);
}

bool CreateNewLogFile(const std::string& log_file_path) {
  std::string old_log_file_path = GetOldLogFileName(log_file_path);
  if (::rename(log_file_path.c_str(), old_log_file_path.c_str()) < 0) {
    LOGE << "Unable to rename log file to old";
    return false;
  }

  std::ofstream tmp(log_file_path.c_str());
  tmp.flush();
  tmp.close();
  return true;
}

std::unordered_map<std::thread::id, std::string> log_file_paths_;

class FileFlusher {
 public:
  FileFlusher() {}
  FileFlusher(const std::string& file) : counter_(0) {
    file_stream_.open(file, std::ios_base::app);
  }

  void Flush(const std::string& message) {
    if (!file_stream_.is_open()) {
      LOGE << "File for logging isn't opened";
      std::cout << message << "\n";
      return;
    }

    if (counter_ == kMaxTimesBeforeFlush) {
      file_stream_.flush();
      counter_ = 0;
      if (GetFileSize(log_file_paths_[std::this_thread::get_id()]) >
                      kMaxLogFileSize) {
        file_stream_.close();
        if (!CreateNewLogFile(log_file_paths_[std::this_thread::get_id()])) {
          LOGE << "Unable to create new log file. Print log to console";
          std::cout << message << "\n";
          return;
        }
        file_stream_.open(log_file_paths_[std::this_thread::get_id()],
                          std::ios_base::app);
      }
    }

    file_stream_ << message << "\n";
    counter_++;
  }

  ~FileFlusher() {
    file_stream_.flush();
    file_stream_.close();
  }

 private:
  uint32_t counter_;
  std::ofstream file_stream_;
};

std::unordered_map<std::thread::id,
                   std::shared_ptr<FileFlusher>> file_flushers_;

std::unique_ptr<std::mutex> console_locker_(new std::mutex());
std::unique_ptr<std::mutex> init_file_locker_(new std::mutex());

const size_t kFileNameLength = 35;
const size_t kLineNumberLength = 3;
const size_t kDateLength = 25;
const size_t kSeverityLength = 11;

std::string ExpandToGivenLength(const std::string& str, size_t length) {
  if (str.size() > length) {
    return str;
  }
  size_t spaces = length - str.size();
  std::string ret = str;
  for (size_t i = 0; i < spaces; ++i) {
    ret += ' ';
  }
  return ret;
}

std::string SetLeadZerosToLineNumber(int line) {
  std::string str = std::to_string(line);
  while (str.size() < kLineNumberLength) {
    str = std::string("0") + str;
  }
  return str;
}

}  // namespace

std::string GetErrorMessage(int error_number) {
  return std::string(" Error: ") +
         ::strerror(error_number) + " - " +
         std::to_string(error_number);
}

LogMessage::LogMessage(const char* filename, int line ,
                       LogSeverity severity, const std::string& appendix,
                       void (*flusher)(const std::string&)) :
      appendix_(std::move(appendix)), flusher_(flusher) {
  message_ << ExpandToGivenLength(GetPathInProject(filename), kFileNameLength)
           << "[" << SetLeadZerosToLineNumber(line) << "] ";
  std::stringstream stream;
  stream << "{" << base::AdvancedTime::Now().GetDate() << "}";
  message_ << ExpandToGivenLength(stream.str(), kDateLength) << " ";
  std::string severity_string = std::string("(") +
                                LogSeverityToString(severity) + "): ";
  message_ << ExpandToGivenLength(severity_string, kSeverityLength);
}

LogMessage::~LogMessage() {
  message_ << " " << appendix_;
  flusher_(message_.str());
}

void InitFileLogger(const std::string& file) {
  ScopedMutex scoped_mutex(init_file_locker_.get());
  file_flushers_[std::this_thread::get_id()] =
      std::make_shared<FileFlusher>(file);
  log_file_paths_[std::this_thread::get_id()] = file;
  scoped_mutex.Release();
  FileLogger::FlushMessage("\n");
}

FileLogger::FileLogger(const char* filename, int line,
                       LogSeverity severity, const std::string& appendix) :
                          LogMessage(filename, line, severity,
                                     appendix, &FileLogger::FlushMessage) {}

// static
void FileLogger::FlushMessage(const std::string& message) {
  std::shared_ptr<FileFlusher> flusher_ptr;
  if (true) {
    ScopedMutex scoped_mutex(init_file_locker_.get());
    flusher_ptr = file_flushers_[std::this_thread::get_id()];
  }
  if (flusher_ptr == nullptr) {
    LOGE << "File Logger isn't initialized";
    return;
  }
  flusher_ptr->Flush(message);
}

ConsoleLogger::ConsoleLogger(const char* filename, int line,
                             LogSeverity severity,
                             const std::string& appendix) :
                                LogMessage(filename, line,
                                           severity, appendix,
                                           &ConsoleLogger::FlushMessage) {}

// static
void ConsoleLogger::FlushMessage(const std::string& message) {
  ScopedMutex scoped_mutex(console_locker_.get());
  std::cout << message << std::endl;
}
