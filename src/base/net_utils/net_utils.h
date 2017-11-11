#ifndef BASE_NET_UTILS_NET_UTILS_H_
#define BASE_NET_UTILS_NET_UTILS_H_

#include <stdint.h>

namespace net_utils {

int CreateEpollFD();
int CreateServerSocket(uint16_t port, uint32_t s_addr);
int AcceptNonblocking(int server_fd);
int CreateExternalServerSocket(const char* host_name, const char* port);

}  // namespace net_utils

#endif  // BASE_NET_UTILS_NET_UTILS_H_
