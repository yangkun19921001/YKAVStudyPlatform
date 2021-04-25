cmake_minimum_required(VERSION 3.15)
project(avcore)

set(CMAKE_CXX_STANDARD 20)

add_executable(avcore parser/h264_parser.c)