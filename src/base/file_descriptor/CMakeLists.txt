project(FileDescriptorLibrary)

include_directories(../)

set(SOURCES file_descriptor.cpp)
set(HEADERS file_descriptor.h)

add_library(file_descriptor_lib ${HEADERS} ${SOURCES})
