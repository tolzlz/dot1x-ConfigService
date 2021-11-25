cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_CXX_STANDARD 11)

add_executable(server wiredstart.cpp wiredclient.cpp)
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})

#添加编译选项
TARGET_LINK_LIBRARIES(server pthread)
set(CMAKE_CXX_FLAGS_DEBUG  " -ggdb ")