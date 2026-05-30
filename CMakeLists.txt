cmake_minimum_required(VERSION 3.10)
project(TicketSystem2026 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Project headers for your ticket system implementation
include_directories(
  ${CMAKE_SOURCE_DIR}/include
)

# STLite header-only containers
add_library(stlite_headers INTERFACE)
target_include_directories(stlite_headers INTERFACE
  ${CMAKE_SOURCE_DIR}/STLite/vector/src
  ${CMAKE_SOURCE_DIR}/STLite/deque
  ${CMAKE_SOURCE_DIR}/STLite/priority_queue/include
  ${CMAKE_SOURCE_DIR}/STLite/map/src
)

# Locate project source files. Place your implementation under src/.
file(GLOB_RECURSE TICKET_SYSTEM_SOURCES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.cc
  ${CMAKE_SOURCE_DIR}/src/*.cxx
)

if(TICKET_SYSTEM_SOURCES)
  add_executable(code ${TICKET_SYSTEM_SOURCES})
  target_link_libraries(code PRIVATE stlite_headers)
  target_include_directories(code PRIVATE ${CMAKE_SOURCE_DIR}/include)
else()
  message(STATUS "No C++ sources found in ${CMAKE_SOURCE_DIR}/src. Add src/main.cpp or src/code.cpp to build the executable named 'code'.")
endif()
