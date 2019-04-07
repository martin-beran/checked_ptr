cmake_minimum_required(VERSION 3.12)

project(checked_ptr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fdiagnostics-color=always")

find_package(Threads REQUIRED)

add_executable(example1 examples/example1.cpp)
target_include_directories(example1 PRIVATE include)
target_link_libraries(example1 Threads::Threads)
