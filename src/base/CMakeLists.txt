project(BaseLibrary)

include_directories(exceptions)
include_directories(file_descriptor)
include_directories(net_utils)
include_directories(time)
include_directories(thread_pool)

set(LOGGER_SOURCES logger.cpp time/advanced_time.cpp scoped_mutex.cpp)
set(LOGGER_HEADERS logger.h time/advanced_time.h scoped_mutex.h)

add_library(logger_lib ${LOGGER_HEADERS} ${LOGGER_SOURCES})


set(SOURCES)
set(HEADERS macros.h time/timer_container.h exceptions/terminal_error.h id_generator.h)

add_library(base_lib ${HEADERS} ${SOURCES})

add_subdirectory(file_descriptor)
add_subdirectory(net_utils)
add_subdirectory(time)
add_subdirectory(thread_pool)

target_link_libraries(base_lib file_descriptor_lib logger_lib net_utils_lib time_lib thread_pool_lib)
