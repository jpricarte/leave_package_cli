cmake_minimum_required(VERSION 3.2)
project(leave_package_cli)

set(CMAKE_CXX_STANDARD 20)

add_executable(leave_package_cli main.cpp lp_exceptions.h communication.cpp communication.h)
target_link_libraries(leave_package_cli pthread)