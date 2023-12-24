cmake_minimum_required(VERSION 3.22)
project(projet_software_security C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl  -std=c++11")

include_directories(.
Client
Server)

add_executable(main
        Client/main.c
        Client/client.h
        Client/server.h
        Server/main.c
        Server/client.h
        Server/server.h
        Client/mainsimple.c
        Server/mainsimple.c
)