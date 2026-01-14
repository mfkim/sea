cmake_minimum_required(VERSION 4.1)
project(sea C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

set(SOURCES
        src/main.c
        src/server.c
        src/http.c
        src/file.c
)

add_executable(sea ${SOURCES})

# 스레드 라이브러리 연결 (Linux/Mac)
find_package(Threads REQUIRED)
target_link_libraries(sea Threads::Threads)

# 소켓 라이브러리 연결 (Windows)
if (WIN32)
    target_link_libraries(sea ws2_32)
endif ()
