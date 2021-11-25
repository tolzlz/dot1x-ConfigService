# cmake_minimum_required(VERSION <specify CMake version here>)
project(BaseAuth2)

set(CMAKE_CXX_STANDARD 14)

add_executable(BaseAuth2 wiredclient.cpp wiredclient.h wiredstart.cpp mxdhcp.cpp mxdhcp.h)
set(CMAKE_C_FLAGS_DEBUG "-ggdb")