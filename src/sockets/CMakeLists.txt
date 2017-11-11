project(SocketsLibrary)

include_directories(../base)
include_directories(../base/exceptions)
include_directories(../base/file_descriptor)
include_directories(../base/net_utils)
include_directories(../base/time)
include_directories(../base/thread_pool)
include_directories(../epoll)

set(SOURCES server_socket.cpp client_socket.cpp external_server_socket.cpp)
set(HEADERS server_socket.h client_socket.h external_server_socket.h)

add_library(sockets_lib ${HEADERS} ${SOURCES})
