cmake_minimum_required(VERSION 2.8.4)
project(XBeeTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)

include_directories(serial/include)

add_subdirectory(serial)
add_executable(XBeeTest ${SOURCE_FILES})
target_link_libraries(XBeeTest serial)