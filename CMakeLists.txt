cmake_minimum_required(VERSION 4.2)
project(webserver C)

set(CMAKE_C_STANDARD 23)

add_executable(webserver
        src/main.c

        src/request.c
        src/client.c
        src/server.c
        src/tpool.c
        src/dynstr.c

        include/client.h
        include/request.h
        include/server.h
        include/tpool.h
        include/dynstr.h
)

include_directories(webserver include/)