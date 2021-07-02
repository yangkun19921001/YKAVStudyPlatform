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
if (NOT DEFINED $ENV{FFMPEG_HOME})
    message(没有找到 FFMPEG_HOME)
    set(FFMPEG_HOME /usr/local/Cellar/ffmpeg/4.3.1_8)
endif ()

set(FFMPEG_PREFIX ${FFMPEG_HOME})
set(FFMPEG_INCLUDE_DIRS ${FFMPEG_PREFIX}/include/)
set(FFMPEG_LIBRARIES ${FFMPEG_PREFIX}/lib/)

include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/)

message(STATUS "ffmpeg include dir is ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "ffmpeg libraries are ${FFMPEG_LIBRARIES}")

set(FFMPEG_DECODE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/decode.cpp)
set(FFMPEG_MUXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/muxer.cpp)
set(FFMPEG_TEMPO_MUXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/tempo_muxer.cpp)

############################   解码   ###############################
add_library(decode SHARED ${FFMPEG_DECODE_SRC})
add_executable(decode-debug ${FFMPEG_DECODE_SRC})
target_link_libraries(decode avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(decode-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )

############################   封装   ###############################
#add_library(muxer SHARED ${FFMPEG_MUXER_SRC})
#add_executable(muxer-debug ${FFMPEG_MUXER_SRC})
#target_link_libraries(muxer avcodec avdevice avfilter avformat avutil swscale swresample
#        )
#target_link_libraries(muxer-debug
#        avcodec avdevice avfilter avformat avutil swscale swresample
#        )


############################   倍数合成   ###############################
add_library(tempo_muxer SHARED ${FFMPEG_TEMPO_MUXER_SRC} ${FFMPEG_MUXER_SRC} ffmpeg/h264ToMp4.cpp ffmpeg/h264ToMp4.h ffmpeg/pqmuxer.cpp ffmpeg/pqmuxer.h)
add_executable(tempo_muxer-debug ${FFMPEG_TEMPO_MUXER_SRC} ${FFMPEG_MUXER_SRC} ffmpeg/h264ToMp4.cpp ffmpeg/h264ToMp4.h ffmpeg/pqmuxer.cpp ffmpeg/pqmuxer.h)
target_link_libraries(tempo_muxer avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(tempo_muxer-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )