#ifndef __LOGGING__
#define __LOGGING__

#define LOG_TAG "ANDRUDIO_JNI"
#define USE_COLOR 1

#define AS_DEBUG_TRACE 16
#define AS_DEBUG_DEBUG 8
#define AS_DEBUG_INFO 4
#define AS_DEBUG_WARN 2
#define AS_DEBUG_ERROR 1

#define AS_DEBUG_LEVEL_NONE 0
#define AS_DEBUG_LEVEL_TRACE 31
#define AS_DEBUG_LEVEL_DEBUG 15
#define AS_DEBUG_LEVEL_INFO 7
#define AS_DEBUG_LEVEL_WARN 3
#define AS_DEBUG_LEVEL_ERROR 1
#define AS_DEBUG_LEVEL_ALL AS_DEBUG_LEVEL_DEBUG

#ifndef AS_DEBUG_LEVEL
#define AS_DEBUG_LEVEL AS_DEBUG_LEVEL_ALL
#endif

#ifdef USE_COLOR
#define COLOR_ERROR_BEGIN "\x1b[31m"
#define COLOR_WARN_BEGIN "\x1b[33m"
#define COLOR_INFO_BEGIN "\x1b[32m"
#define COLOR_DEBUG_BEGIN "\x1b[36m"
#define COLOR_TRACE_BEGIN "\x1b[35m"
#define COLOR_END "\x1b[0m"
#else
#define COLOR_ERROR_BEGIN
#define COLOR_WARN_BEGIN
#define COLOR_INFO_BEGIN
#define COLOR_DEBUG_BEGIN
#define COLOR_TRACE_BEGIN
#define COLOR_END
#endif

#ifdef __ANDROID__

#include <android/log.h>

#define ANDROID_PRINT __android_log_print
#define log_error(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_ERROR) ANDROID_PRINT(ANDROID_LOG_ERROR, LOG_TAG, COLOR_ERROR_BEGIN format COLOR_END,## __VA_ARGS__)
#define log_warn(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_WARN)   ANDROID_PRINT(ANDROID_LOG_WARN, LOG_TAG, COLOR_WARN_BEGIN format COLOR_END,## __VA_ARGS__)
#define log_info(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_INFO)   ANDROID_PRINT(ANDROID_LOG_INFO, LOG_TAG, COLOR_INFO_BEGIN format COLOR_END,## __VA_ARGS__)
#define log_debug(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_DEBUG) ANDROID_PRINT(ANDROID_LOG_DEBUG, LOG_TAG, COLOR_DEBUG_BEGIN format COLOR_END,## __VA_ARGS__)
#define log_trace(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_TRACE) ANDROID_PRINT(ANDROID_LOG_VERBOSE, LOG_TAG, COLOR_TRACE_BEGIN format COLOR_END,## __VA_ARGS__)
#else
#define _log(output,prefix,format, ...) fprintf(output,prefix "%s:%d:\t" format COLOR_END"\n",__FILE__,__LINE__,## __VA_ARGS__)
#define log_trace(format, ...) if(AS_DEBUG_LEVEL&AS_DEBUG_TRACE)_log(stdout,COLOR_TRACE_BEGIN"TRACE:", format, ## __VA_ARGS__)
#define log_debug(format, ...) if (AS_DEBUG_LEVEL & AS_DEBUG_DEBUG) _log(stdout,COLOR_DEBUG_BEGIN"DEBUG:", format, ## __VA_ARGS__)
#define log_info(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_INFO)  _log(stdout,COLOR_INFO_BEGIN"INFO:" ,format, ## __VA_ARGS__)
#define log_warn(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_WARN)  _log(stdout,COLOR_WARN_BEGIN"WARN:" ,format ,## __VA_ARGS__)
#define log_error(format, ...) if (AS_DEBUG_LEVEL & AS_DEBUG_ERROR)  _log(stderr,COLOR_ERROR_BEGIN"ERROR:" ,format, ## __VA_ARGS__)
#endif //__ANDROID__


#endif
