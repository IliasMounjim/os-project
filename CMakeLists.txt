cmake_minimum_required(VERSION 3.30)

project(schedulerSim)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(external/jsoncpp)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PUBLIC
      src/schedule.cpp
      src/policy.cpp
      src/fcfs.cpp
      src/sjf.cpp
      src/srtf.cpp
      src/ljf.cpp
      src/rr.cpp
      src/lottery.cpp
      src/priority.cpp
      src/hybrid.cpp
      src/main.cpp
      
    PRIVATE
    FILE_SET HEADERS
    FILES
      src/schedule.h
      src/policy.h
      src/fcfs.h
      src/srtf.h
      src/ljf.h
      src/rr.h
      src/lottery.h
      src/priority.h
      src/hybrid.h
)

# json includes
# Source - https://stackoverflow.com/a/72442751 
# Posted by jignatius, modified by community. See post 'Timeline' for change history 
# Retrieved 2026-04-08, License - CC BY-SA 4.0
get_target_property(JSON_INC_PATH jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${JSON_INC_PATH})
target_link_libraries(${PROJECT_NAME} jsoncpp_lib)
