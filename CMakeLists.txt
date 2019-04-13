cmake_minimum_required(VERSION 3.12)

project(checked_ptr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fdiagnostics-color=always")

find_package(Threads REQUIRED)

add_executable(example1 examples/example1.cpp)

target_include_directories(example1 PRIVATE include)
target_link_libraries(example1 Threads::Threads)

set(TARGET benchmark)
foreach (VARIANT shared checked_shared checked_weak checked_raw)
    set(T ${TARGET}_${VARIANT}_ptr)
    string(TOUPPER ${VARIANT} VARIANT_U)
    add_executable(${T} examples/benchmark.cpp)
    target_include_directories(${T} PRIVATE include)
    target_link_libraries(${T} Threads::Threads)
    target_compile_definitions(${T} PRIVATE -D${VARIANT_U}_PTR)
endforeach()



