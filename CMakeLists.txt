cmake_minimum_required(VERSION 3.15)
#多目录工程的CmakeLists.txt编写（自动添加多目录下的文件）
project(ykavstudyplatform)


#add_executable(ykavstudyplatform MACOSX_BUNDLE ${src})          #编译为可执行程序 *.app

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)       #设置可执行文件的输出目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)           #设置库文件的输出目录
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")         # Release模式下的编译指令

#加载码流分析
include_directories(./avcore/bsparser)
add_subdirectory(./avcore/bsparser)

#加载 media
add_subdirectory(avcore/media)
#加载 qt
add_subdirectory(avcore/qt)

#C++ 实现消息队列
add_subdirectory(avcore/looper)

#加载 demo 示例
aux_source_directory(./sample DEMO)
add_executable(ykavstudyplatform ${DEMO})

target_link_libraries(ykavstudyplatform bsparser)




