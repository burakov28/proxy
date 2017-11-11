project(ThreadPoolLibrary)

include_directories(../)
include_directories(../exceptions)

set(SOURCES thread_pool.cpp)
set(HEADERS thread_pool.h)

add_library(thread_pool_lib ${HEADERS} ${SOURCES})
