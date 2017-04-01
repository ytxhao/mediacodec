cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -UNDEBUG")

add_library(native-ytx-lib SHARED
            src/looper.cpp
            src/lock.cpp
            src/HybridMediaPlayer.cpp
            src/android_media_YtxMediaPlayer.cpp
            )

# Include libraries needed for native-codec-jni lib
target_link_libraries(native-ytx-lib
                      android
                      log
                      mediandk
                      OpenMAXAL
                      GLESv2
                      EGL)

