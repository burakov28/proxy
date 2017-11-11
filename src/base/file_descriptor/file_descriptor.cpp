#include "file_descriptor.h"

#include <unistd.h>

#include <utility>
#include <string>

#include "logger.h"

namespace base {

FileDescriptor::FileDescriptor() : fd_(-1) {}
FileDescriptor::FileDescriptor(int fd) : fd_(fd) {}

FileDescriptor::FileDescriptor(FileDescriptor&& other) : fd_(other.fd_) {
  other.fd_ = -1;
}

FileDescriptor::~FileDescriptor() {
  Close();
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) {
  if (fd_ == other.fd_) {
    return *this;
  }

  Close();
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}

bool FileDescriptor::IsValid() const {
  return fd_ >= 0;
}

void FileDescriptor::Close() {
  if (fd_ < 0) {
    return;
  } else if (::close(fd_) < 0) {
    LOGE << "Unable to close file descriptor: " << fd_;
  }
  fd_ = -1;
}

int FileDescriptor::Release() {
  int fd = fd_;
  fd_ = -1;
  return fd;
}

int FileDescriptor::GetFD() const {
  return fd_;
}

IOFileDescriptor::IOFileDescriptor() {}
IOFileDescriptor::IOFileDescriptor(int fd, size_t max_buffer_size) :
    FileDescriptor(fd), max_buffer_size_(max_buffer_size) {}

IOFileDescriptor::IOFileDescriptor(IOFileDescriptor&& other) :
    FileDescriptor(std::move(static_cast<FileDescriptor&>(other))),
    buffer_(std::move(other.buffer_)),
    max_buffer_size_(other.max_buffer_size_) {}

IOFileDescriptor& IOFileDescriptor::operator=(IOFileDescriptor&& other) {
  FileDescriptor::operator=(std::move(static_cast<FileDescriptor&>(other)));
  buffer_.clear();
  buffer_ = std::move(other.buffer_);
  return *this;
}

bool IOFileDescriptor::Append(const char* data, size_t size) {
  if (max_buffer_size_ - buffer_.size() < size) {
    LOGE << "Buffer is overflowed for fd: " << GetFD();
    return false;
  }
  buffer_.append(data, size);
  return true;
}

std::string IOFileDescriptor::Read() {
  int was_read = ::read(GetFD(), read_buffer_, kReadBufferSize - 1);
  if (was_read < 0) {
    LOGE << "Reading error for fd: " << GetFD();
    return "";
  }
  return std::string(read_buffer_, was_read);
}

bool IOFileDescriptor::Write() {
  int was_written = ::write(GetFD(), buffer_.c_str(), buffer_.size());
  if (was_written <= 0) {
    LOGE << "Writing error for fd: " << GetFD();
    return false;
  }
  if (was_written > 0) {
    for (size_t i = was_written; i < buffer_.size(); ++i) {
      buffer_[i - was_written] = buffer_[i];
    }
    buffer_.resize(buffer_.size() - was_written);
  }
  return true;
}

size_t IOFileDescriptor::GetSize() const {
  return buffer_.size();
}

bool IOFileDescriptor::IsEmpty() const {
  return GetSize() == 0;
}

}  // namespace base
