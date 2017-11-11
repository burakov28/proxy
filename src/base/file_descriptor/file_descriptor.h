#ifndef BASE_FILE_DESCRIPTOR_FILE_DESCRIPTOR_H_
#define BASE_FILE_DESCRIPTOR_FILE_DESCRIPTOR_H_

#include <string>

#include "macros.h"

namespace base {

class FileDescriptor {
 public:
  FileDescriptor();
  explicit FileDescriptor(int fd);

  FileDescriptor(FileDescriptor&& other);
  FileDescriptor& operator=(FileDescriptor&& other);

  ~FileDescriptor();

  void Close();
  int Release();
  bool IsValid() const;
  int GetFD() const;

 private:
  int fd_;

  PROHIBIT_COPY_AND_COPY_ASSIGN(FileDescriptor);
};

class IOFileDescriptor : public FileDescriptor {
 public:
  IOFileDescriptor();
  IOFileDescriptor(int fd, size_t max_buffer_size = kMaxBufferSize);

  IOFileDescriptor(IOFileDescriptor&& other);
  IOFileDescriptor& operator=(IOFileDescriptor&& other);

  bool Append(const char* data, size_t size);
  bool Write();

  std::string Read();

  size_t GetSize() const;
  bool IsEmpty() const;

 private:
  static const size_t kMaxBufferSize = 128 * 1024;
  static const size_t kReadBufferSize = 4 * 1024;

  std::string buffer_;
  size_t max_buffer_size_;
  char read_buffer_[kReadBufferSize];

  PROHIBIT_COPY_AND_COPY_ASSIGN(IOFileDescriptor);
};

}  // namespace base

#endif  // BASE_FILE_DESCRIPTOR_FILE_DESCRIPTOR_H_
