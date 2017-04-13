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
            src/frame_queue.cpp
            src/frame_queue_video.cpp
            src/VideoRefreshController.cpp
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



add_library( harfbuzz-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       harfbuzz-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libharfbuzz.so )

add_library( freetype-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       freetype-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libfreetype.so )


add_library( iconv-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       iconv-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libiconv.so )


add_library( xml2-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       xml2-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libxml2.so )

add_library( fontconfig-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       fontconfig-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libfontconfig.so )


add_library( fribidi-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       fribidi-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libfribidi.so )

add_library( ass-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       ass-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libass.so )


add_library( avutil-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       avutil-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libavutil.so )

add_library( swscale-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       swscale-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libswscale.so )

add_library( swresample-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       swresample-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libswresample.so )


add_library( avcodec-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       avcodec-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libavcodec.so )

add_library( avformat-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       avformat-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libavformat.so )

add_library( avfilter-lib
             SHARED
             IMPORTED )

set_target_properties( # Specifies the target library.
                       avfilter-lib

                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION

                       # Provides the path to the library you want to import.
                       ${LINK_DIR}/libavfilter.so )


# Include libraries needed for native-codec-jni lib
target_link_libraries(native-ytx-lib
                      native-gl-lib
                      android
                      log
                      mediandk
                      OpenMAXAL
                      GLESv2
                      EGL
                      png-lib
                      harfbuzz-lib
                      freetype-lib
                      iconv-lib
                      xml2-lib
                      fontconfig-lib
                      fribidi-lib
                      ass-lib
                      avutil-lib
                      swscale-lib
                      swresample-lib
                      avcodec-lib
                      avformat-lib
                      avfilter-lib)

