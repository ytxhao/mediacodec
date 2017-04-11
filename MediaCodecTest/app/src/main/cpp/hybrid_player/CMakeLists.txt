cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -UNDEBUG")

add_library(native-ytx-lib SHARED
            src/looper.cpp
            src/lock.cpp
            src/HybridMediaPlayer.cpp
            src/android_media_YtxMediaPlayer.cpp
            src/GlslFilter.cpp
            src/MessageQueue.cpp
            src/VideoStateInfo.cpp
            src/Thread.cpp
            src/GLThread.cpp
            )


set(LINK_DIR "${Project_SOURCE_DIR}/../jniLibs/armeabi-v7a")
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library( png-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       png-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libpng16.so )

# Include libraries needed for native-codec-jni lib
target_link_libraries(native-ytx-lib
                      png-lib
                      android
                      log
                      mediandk
                      OpenMAXAL
                      GLESv2
                      EGL)

