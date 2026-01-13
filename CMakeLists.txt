cmake_minimum_required(VERSION 4.1)
project(sea C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(sea src/main.c)

if (WIN32)
    target_link_libraries(sea ws2_32)
endif ()
