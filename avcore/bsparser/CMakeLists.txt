cmake_minimum_required(VERSION 3.15)
project(bsparser)

#分别设置了Debug版本和Release版本可执行文件的输出目录,
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin)

#分别设置了Debug版本和Release版本库文件的输出目录,
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../lib)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../lib)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../bin)       #设置可执行文件的输出目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../lib)           #设置库文件的输出目录

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")         # Release模式下的编译指令

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 11)

#编译成动态库
add_library(bsparser SHARED

        NalParse.cpp

        h264_sei.cpp
        h264_avcc.cpp
        h264_sei.cpp
        h264_stream.cpp
        h264_analyze.cpp

        h265_avcc.cpp
        h265_sei.cpp
        h265_stream.cpp
        )
#编译可执行文件
add_executable(bsparser-debug

        NalParse.cpp

        h264_sei.cpp
        h264_avcc.cpp
        h264_sei.cpp
        h264_stream.cpp
        h264_analyze.cpp

        h265_avcc.cpp
        h265_sei.cpp
        h265_stream.cpp
        main.cpp)
