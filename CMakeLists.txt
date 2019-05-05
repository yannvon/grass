cmake_minimum_required(VERSION 3.13)
project(Grass)

set(CMAKE_CXX_STANDARD 14)
find_package (Threads)

include_directories(src)

add_executable(GrassClient
        src/client.cpp src/User.cpp src/User.hpp src/networking.hpp src/networking.cpp src/grass.cpp src/grass.hpp)

add_executable(GrassServer
        src/server.cpp src/Parser.cpp src/Parser.hpp src/commands.hpp src/commands.cpp src/User.cpp src/User.hpp src/networking.hpp src/networking.cpp src/grass.cpp src/grass.hpp )


target_link_libraries (GrassServer ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (GrassClient ${CMAKE_THREAD_LIBS_INIT})