project(future)

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

add_executable(future main.cpp Channel.h future.h Client.h Client.cpp Server.h Server.cpp)
if (NOT WIN32)
  target_link_libraries(future pthread)
endif()
