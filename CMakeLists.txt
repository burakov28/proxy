cmake_minimum_required(VERSION 2.8)
project(proxy)

include_directories(src/base)
include_directories(src/base/exceptions)
include_directories(src/base/file_descriptor)
include_directories(src/base/net_utils)
include_directories(src/base/thread_pool)
include_directories(src/base/time)
include_directories(src/epoll)
include_directories(src/sockets)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2 -Wall -pthread")

set(SRC_LIST src/main.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})

set(LIBS base_lib sockets_lib epoll_lib)
add_subdirectory(src/base)
add_subdirectory(src/epoll)
add_subdirectory(src/sockets)
target_link_libraries(${PROJECT_NAME} ${LIBS})
