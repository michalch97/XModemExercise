cmake_minimum_required(VERSION 3.10)
project(XModemExcercise)

set(CMAKE_CXX_STANDARD 11)
#set (CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ -static-libwinpthread")
set (CMAKE_CXX_FLAGS "-static")

add_executable(XModemExcercise main.cpp functions.cpp)