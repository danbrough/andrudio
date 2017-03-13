# Sets the minimum version of CMake required to build the native
# library. You should either keep the default value or only pass a
# value of 3.4.0 or lower.

cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds it for you.
# Gradle automatically packages shared libraries with your APK.

set(ffmpeg_libs ${CMAKE_SOURCE_DIR}/../ffmpeg/libs)
include_directories( ${CMAKE_SOURCE_DIR}/../ffmpeg/include/ )

add_library( libavutil SHARED IMPORTED )
set_target_properties( libavutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/${ANDROID_ABI}/libavutil.so )
add_library( libavcodec SHARED IMPORTED )
set_target_properties( libavcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/${ANDROID_ABI}/libavcodec.so )
add_library( libavformat SHARED IMPORTED )
set_target_properties( libavformat PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/${ANDROID_ABI}/libavformat.so )
add_library( libavresample SHARED IMPORTED )
set_target_properties( libavresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_libs}/${ANDROID_ABI}/libavresample.so )

add_library( # Sets the name of the library.
             andrudio
             SHARED
             src/main/native/andrudio.c
             src/main/native/audioplayer.c
             src/main/native/player_thread.c
              )

find_library( log-lib log )

target_link_libraries( andrudio libavutil libavcodec libavformat libavresample ${log-lib} )



