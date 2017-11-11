project(NetUtilsLibrary)

include_directories(../)
include_directories(../exceptions)
include_directories(../file_descriptor)

set(SOURCES net_utils.cpp http_parser.cpp)
set(HEADERS net_utils.h http_parser.h)

add_library(net_utils_lib ${HEADERS} ${SOURCES})
