cmake_minimum_required(VERSION 3.15)
project(media)
set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeModules)
#find_package(FFMPEG REQUIRED)
#if (NOT FFMPEG_FOUND)
#    message(ERROR "Cannot find ffmpeg")
#else ()
#    message(STATUS "Found ffmpeg")
#    message(STATUS "ffmpeg include dir is ${FFMPEG_INCLUDE_DIRS}")
#    message(STATUS "ffmpeg libraries are ${FFMPEG_LIBRARIES}")
#endif ()

set(FFMPEG_PREFIX /usr/local/Cellar/ffmpeg/4.3.1_8)
#set(FFMPEG_PREFIX /Users/devyk/Data/qt/project/QTFFPlayer/libs)
set(FFMPEG_INCLUDE_DIRS ${FFMPEG_PREFIX}/include/)
set(FFMPEG_LIBRARIES ${FFMPEG_PREFIX}/lib/)

include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/)

message(STATUS "ffmpeg include dir is ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "ffmpeg libraries are ${FFMPEG_LIBRARIES}")

set(FFMPEG_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/decode.cpp)

add_library(media SHARED ${FFMPEG_SRC})
add_executable(media-debug ${FFMPEG_SRC})
target_link_libraries(media avcodec avdevice avfilter avformat avutil swscale swresample
        )

target_link_libraries(media-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
