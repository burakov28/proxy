project(TimeLibrary)

set(SOURCES advanced_time.cpp)
set(HEADERS advanced_time.h)

add_library(time_lib ${HEADERS} ${SOURCES})
