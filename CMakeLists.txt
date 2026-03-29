cmake_minimum_required(VERSION 3.30)

project(Scheduler_sim)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(schedulerSim)
target_sources(schedulerSim
    PUBLIC
      src/schedule.cpp
      src/policy.cpp
      src/fcfs.cpp
      src/sjf.cpp
      src/srtf.cpp
      src/ljf.cpp
      src/rr.cpp
      src/lottery.cpp
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
      src/hybrid.h
)
