#include "net_utils.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <thread>

#include "file_descriptor.h"
#include "logger.h"
#include "terminal_error.h"

namespace net_utils {

const size_t kAttemptsToCreateEpoll = 5;
const size_t kAttemptsToCreateServerSocket = 5;
const uint64_t kSleepBeforeNextAttemptMs = 2000;

int CreateEpollFD() {
  for (size_t i = 0; i < kAttemptsToCreateEpoll; ++i) {
    int fd = ::epoll_create1(0);
    if (fd <= 0) {
      LOGE << "Error to create epoll's fd. Attempt: " << i + 1 << " failed"
           << "; sleep_for " << kSleepBeforeNextAttemptMs << "ms";
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kSleepBeforeNextAttemptMs));
      continue;
    }
    return fd;
  }
  return -1;
}



int CreateServerSocket(uint16_t port, uint32_t s_addr) {
  int socket_fd;
  for (size_t i = 0; i < kAttemptsToCreateServerSocket; ++i) {
    socket_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (socket_fd <= 0) {
      LOGE << "Error create server socket. Attempt: " << i + 1 << " failed"
           << "; sleep_for " << kSleepBeforeNextAttemptMs << "ms";
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kSleepBeforeNextAttemptMs));
      continue;
    }

    base::FileDescriptor scoped_fd(socket_fd);

    ::sockaddr_in addr;
    memset(&addr, 0, sizeof(::sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = s_addr;

    if (::bind(socket_fd,
               (::sockaddr*) (&addr),
               sizeof(::sockaddr_in)) < 0) {
      LOGE << "Error to bind server socket: " << socket_fd
           << ". Attempt: " << i + 1 << " failed!";
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kSleepBeforeNextAttemptMs));
      continue;
    }

    if (::listen(socket_fd, 128) < 0) {
      LOGE << "Error to start to listen. Attempt: " << i + 1 << " failed!";
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kSleepBeforeNextAttemptMs));
      continue;
    }

    return scoped_fd.Release();
  }

  LOGE << "TERMINATION_ERROR: Can't create server socket.";
  throw TerminalError();
}

int AcceptNonblocking(int server_fd) {
  ::sockaddr_in addr;
  ::socklen_t addr_len = 0;
  int client_fd = ::accept4(server_fd,
                            (::sockaddr*) (&addr),
                            &addr_len,
                            SOCK_NONBLOCK);
  return client_fd;
}

bool ChangeFileDescriptorFlags(int fd, int flags, std::function<int(int, int)> changer) {
  int old_flags = ::fcntl(fd, F_GETFD);
  if (old_flags == -1) {
    FLOGE << "Error to retreive flags for fd: " << fd;
    return false;
  }
  if (::fcntl(fd, F_SETFD, changer(old_flags, flags)) == -1) {
    FLOGE << "Error to set flags for fd: " << fd;
    return false;
  }
  return true;
}

bool AddFileDescriptorFlags(int fd, int flags) {
  if (!ChangeFileDescriptorFlags(fd, flags, [](int a, int b) {
    return a | b; })) {
    FLOGE << "Error to add flag to fd: " << fd;
    return false;
  }
  return true;
}

bool RemoveFileDescriptorFlags(int fd, int flags) {
  if (!ChangeFileDescriptorFlags(fd, flags, [](int a, int b) {
    return a & (~b); })) {
    FLOGE << "Error to remove flag to fd: " << fd;
    return false;
  }
  return true;
}

int CreateExternalServerSocket(const char* host_name, const char* port) {
  FLOGI << "Resolve name: " << host_name
        << "; port: " << port;
  ::addrinfo hints;
  memset(&hints, 0, sizeof(::addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  ::addrinfo* result = nullptr;
  if (::getaddrinfo(host_name, port, &hints, &result) != 0) {
    FLOGE << "Couldn't resolve host name: " << host_name << ":" << port;
    return -1;
  }
  FLOGI << "Resolve complete successfully";

  for (::addrinfo* p = result; p != NULL; p = p->ai_next) {
    int socket_fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (socket_fd <= 0) {
      FLOGE << "Error to create socket from resolved name. Next entry";
      continue;
    }
    FLOGI << "Create socket; fd: " << socket_fd;
    base::FileDescriptor scoped_socket(socket_fd);
    if (!RemoveFileDescriptorFlags(socket_fd, SOCK_NONBLOCK)) {
      FLOGE << "Error to remove nonblock from socket: "
            << socket_fd << ". Next entry";
      continue;
    }
    FLOGI << "Make socket blocking successfully";
    if (::connect(socket_fd, p->ai_addr, p->ai_addrlen) < 0) {
      FLOGE << "Error to connect to server; fd: "
            << socket_fd << ". Next entry";
      continue;
    }
    if (!AddFileDescriptorFlags(socket_fd, SOCK_NONBLOCK)) {
      FLOGE << "Error to make socket: " << socket_fd
            << " nonblock. Next entry";
      continue;
    }
    FLOGI << "Socket connection is successful. fd: " << socket_fd
          << " was connected to " << host_name
          << " port: " << ((port) ? (port) : (""))
          << ". By IP: " << inet_ntoa(((::sockaddr_in*) p->ai_addr)->sin_addr);
    scoped_socket.Release();
    ::freeaddrinfo(result);
    return socket_fd;
  }
  ::freeaddrinfo(result);

  FLOGE << "Error to connect to any address: " << host_name
        << "; port: " << ((port) ? (port) : (""));
  return -1;
}

}  // namespace net_utils
