project(EpollLibrary)

include_directories(../base)
include_directories(../base/exceptions)
include_directories(../base/file_descriptor)
include_directories(../base/net_utils)
include_directories(../base/time)

set(SOURCES epoll.cpp epoll_record.cpp signal_handler.cpp epoll_notifier.cpp)
set(HEADERS epoll.h epoll_record.h signal_handler.h epoll_notifier.h)

add_library(epoll_lib ${HEADERS} ${SOURCES})
