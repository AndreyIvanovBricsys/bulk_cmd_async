cmake_minimum_required(VERSION 3.10)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

project(async)
add_subdirectory(src)

install(TARGETS async RUNTIME DESTINATION bin)