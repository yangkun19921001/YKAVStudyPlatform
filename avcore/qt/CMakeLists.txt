cmake_minimum_required(VERSION 3.15)
project(qt_core)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH "/Users/devyk/Data/qt/install/5/5.14.2/5.14.2/clang_64/lib/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_VERSION 5)
set(REQUIRED_LIBS Core Gui Widgets Multimedia)
set(REQUIRED_LIBS_QUALIFIED Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia)

if (NOT CMAKE_PREFIX_PATH)
    message(WARNING "CMAKE_PREFIX_PATH is not defined, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/audio)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/video)

set(QT_AUDIO_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/audio/QAudioPlayer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/audio/main.cpp audio/pcmplay.cpp)

add_executable(qt-audio-debug ${QT_AUDIO_SRC})
target_link_libraries(qt-audio-debug ${REQUIRED_LIBS_QUALIFIED})
