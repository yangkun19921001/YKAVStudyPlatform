cmake_minimum_required(VERSION 3.15)

set(alooper
        src/aloop.cpp
        src/aloop.h
        example/main.cpp

#        hanlder/Looper.cpp
#        hanlder/threads.cpp
#        hanlder/utils.cpp
#        hanlder/WkHandler.cpp
#        hanlder/WKLooper.cpp
#        hanlder/WKMessage.cpp
#        hanlder/WKMessageQueue.cpp
#        hanlder/WKMessageTest.cpp
        )

#include_directories(hanlder/include)

add_executable(looper ${alooper})
target_link_libraries(looper)