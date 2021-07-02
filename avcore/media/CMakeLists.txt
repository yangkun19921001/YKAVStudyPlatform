cmake_minimum_required(VERSION 3.15)
project(media)
set(CMAKE_CXX_STANDARD 11)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeModules)
#find_package(FFMPEG REQUIRED)
#if (NOT FFMPEG_FOUND)
#    message(ERROR "Cannot find muxer")
#else ()
#    message(STATUS "Found muxer")
#    message(STATUS "muxer include dir is ${FFMPEG_INCLUDE_DIRS}")
#    message(STATUS "muxer libraries are ${FFMPEG_LIBRARIES}")
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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/muxer/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utils)
message(STATUS "ffmpeg include dir is ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "ffmpeg libraries are ${FFMPEG_LIBRARIES}")

set(FFMPEG_DECODE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/decode/decode.cpp)
set(FFMPEG_MUXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/muxer/muxer.cpp)
set(FFMPEG_TEMPO_MUXER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/muxer/tempo_muxer.cpp)

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
add_library(tempo_muxer SHARED ${FFMPEG_TEMPO_MUXER_SRC} ${FFMPEG_MUXER_SRC} muxer/h264ToMp4.cpp muxer/h264ToMp4.h muxer/pqmuxer.cpp muxer/pqmuxer.h)
add_executable(tempo_muxer-debug ${FFMPEG_TEMPO_MUXER_SRC} ${FFMPEG_MUXER_SRC} muxer/h264ToMp4.cpp muxer/h264ToMp4.h muxer/pqmuxer.cpp muxer/pqmuxer.h)
target_link_libraries(tempo_muxer avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(tempo_muxer-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )

############################   ffplay 播放器 ###############################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffplay/include/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffplay/lib)
set(FFPLAY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ffplay/ffplay.c ${CMAKE_CURRENT_SOURCE_DIR}/utils/cmdutils.c)
add_executable(ffplay-debug ${FFPLAY_SRC} ${FFMPEG_MUXER_SRC})
target_link_libraries(ffplay-debug
        avcodec avdevice avfilter avformat avutil swscale swresample SDL2
        )
############################   ffmpeg 命令 ###############################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/)
set(FFMPEG_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg_opt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg_filter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg/ffmpeg_hw.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/cmdutils.c
        )
add_executable(ffmpeg-debug ${FFMPEG_SRC})
target_link_libraries(ffmpeg-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )

############################   ffprobe 命令 ###############################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffprobe/)
set(FFPROBE_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/ffprobe/ffprobe.c
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/cmdutils.c
        )
add_executable(ffprobe-debug ${FFPROBE_SRC})
target_link_libraries(ffprobe-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )

############################   ffmpeg-examples 例子 ###############################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/)
add_executable(avio_dir_cmd-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/avio_dir_cmd.c)
add_executable(avio_reading-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/avio_reading.c)
add_executable(decode_audio-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/decode_audio.c)
add_executable(decode_video-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/decode_video.c)
add_executable(demuxing_decoding-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/demuxing_decoding.c)
add_executable(encode_audio-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/encode_audio.c)
add_executable(encode_video-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/encode_video.c)
add_executable(extract_mvs-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/extract_mvs.c)
add_executable(filter_audio-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/filter_audio.c)
add_executable(filtering_audio-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/filtering_audio.c)
add_executable(filtering_video-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/filtering_video.c)
add_executable(http_multiclient-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/http_multiclient.c)
add_executable(hw_decode-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/hw_decode.c)
add_executable(metadata-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/metadata.c)
add_executable(muxing-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/muxing.c)
#add_executable(qsvdec-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/qsvdec.c)
add_executable(remuxing-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/remuxing.c)
add_executable(resampling_audio-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/resampling_audio.c)
add_executable(scaling_video-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/scaling_video.c)
add_executable(transcode_aac-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/transcode_aac.c)
add_executable(transcoding-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/transcoding.c)
add_executable(vaapi_encode-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/vaapi_encode.c)
add_executable(vaapi_transcode-debug ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg_examples/vaapi_transcode.c)

target_link_libraries(avio_dir_cmd-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(transcoding-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(avio_reading-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(vaapi_transcode-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(decode_audio-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(decode_video-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(demuxing_decoding-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(encode_audio-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(encode_video-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(extract_mvs-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(filter_audio-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(filtering_audio-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(filtering_video-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(http_multiclient-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(hw_decode-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(metadata-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(muxing-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
#target_link_libraries(qsvdec-debug
#        avcodec avdevice avfilter avformat avutil swscale swresample
#        )
target_link_libraries(remuxing-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(resampling_audio-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(scaling_video-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(transcode_aac-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(vaapi_encode-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )
target_link_libraries(vaapi_transcode-debug
        avcodec avdevice avfilter avformat avutil swscale swresample
        )


