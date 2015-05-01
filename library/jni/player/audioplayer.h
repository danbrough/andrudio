#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>


#define LOG_TAG "danbroid.andrudio"

#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define FAILURE -1

#define OUTPUT_SAMPLE_FMT AV_SAMPLE_FMT_S16
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
#define AS_DEBUG_LEVEL_ALL AS_DEBUG_LEVEL_TRACE

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
//#include <SLES/OpenSLES_Android.h>

#define ANDROID_PRINT __android_log_print
#define log_error(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_ERROR) ANDROID_PRINT(ANDROID_LOG_ERROR, LOG_TAG, format,## __VA_ARGS__)
#define log_warn(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_WARN)   ANDROID_PRINT(ANDROID_LOG_WARN, LOG_TAG, format,## __VA_ARGS__)
#define log_info(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_INFO)   ANDROID_PRINT(ANDROID_LOG_INFO, LOG_TAG, format,## __VA_ARGS__)
#define log_debug(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_DEBUG) ANDROID_PRINT(ANDROID_LOG_DEBUG, LOG_TAG, format,## __VA_ARGS__)
#define log_trace(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_TRACE) ANDROID_PRINT(ANDROID_LOG_VERBOSE, LOG_TAG, format,## __VA_ARGS__)
#else
#define _log(output,prefix,format, ...) fprintf(output,prefix "%s:%d:\t" format COLOR_END"\n",__FILE__,__LINE__,## __VA_ARGS__)
#define log_trace(format, ...) if(AS_DEBUG_LEVEL&AS_DEBUG_TRACE)_log(stdout,COLOR_TRACE_BEGIN"TRACE:", format, ## __VA_ARGS__)
#define log_debug(format, ...) if (AS_DEBUG_LEVEL & AS_DEBUG_DEBUG) _log(stdout,COLOR_DEBUG_BEGIN"DEBUG:", format, ## __VA_ARGS__)
#define log_info(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_INFO)  _log(stdout,COLOR_INFO_BEGIN"INFO:" ,format, ## __VA_ARGS__)
#define log_warn(format, ...)  if (AS_DEBUG_LEVEL & AS_DEBUG_WARN)  _log(stdout,COLOR_WARN_BEGIN"WARN:" ,format ,## __VA_ARGS__)
#define log_error(format, ...) if (AS_DEBUG_LEVEL & AS_DEBUG_ERROR)  _log(stderr,COLOR_ERROR_BEGIN"ERROR:" ,format, ## __VA_ARGS__)
#endif //__ANDROID__

#include "packet_queue.h"

#define MAX_QUEUE_SIZE (15  * 1024)
#define MIN_AUDIOQ_SIZE (20 * 16 * 1024)
#define SDL_AUDIO_BUFFER_SIZE 1024

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
#define SAMPLE_ARRAY_SIZE (2 * 65536)

typedef enum {
	STATE_IDLE,
	STATE_INITIALIZED,
	STATE_PREPARING,
	STATE_PREPARED,
	STATE_STARTED,
	STATE_PAUSED,
	STATE_COMPLETED,
	STATE_STOPPED,
	STATE_ERROR,
	STATE_END
} audio_state_t;

typedef enum {
	EVENT_THREAD_START = 1, EVENT_STATE_CHANGE, EVENT_SEEK_COMPLETE
} audio_event_t;

const char* ap_get_state_name(audio_state_t state);

typedef struct player_t {
		int looping;
		audio_state_t state;
		audio_state_t last_paused;
		pthread_mutex_t mutex;
		pthread_t read_thread;
		pthread_t play_thread;
		int abort_request;
		int seek_req;
		int seek_flags;
		int64_t seek_pos;
		int64_t seek_rel;
		//int read_pause_return;
		AVFormatContext *ic;

		int audio_stream;

		double audio_clock;

		AVStream *audio_st;
		PacketQueue audioq;
		//int audio_hw_buf_size;
		uint8_t silence_buf[SDL_AUDIO_BUFFER_SIZE];
		uint8_t *audio_buf;
		uint8_t *audio_buf1;
		unsigned int audio_buf_size; /* in bytes */
		int audio_buf_index; /* in bytes */
		AVPacket audio_pkt_temp;
		AVPacket audio_pkt;
		enum AVSampleFormat sdl_sample_fmt;

		uint64_t sdl_channel_layout;
		int sdl_channels;
		int sdl_sample_rate;

		enum AVSampleFormat resample_sample_fmt;
		uint64_t resample_channel_layout;
		int resample_sample_rate;

		AVAudioResampleContext *avr;
		AVFrame *frame;

		int16_t *sample_array[SAMPLE_ARRAY_SIZE];
		int sample_array_index;

		/*
		 #if !CONFIG_AVFILTER
		 struct SwsContext *img_convert_ctx;
		 #endif
		 */

		//    QETimer *video_timer;
		char url[1024];

		struct _player_callbacks_t {

				void (*on_event)(struct player_t *player, audio_event_t event,
				        int arg1, int arg2);

				void (*on_play)(struct player_t *player, char *data, int len);

				int (*on_prepare)(struct player_t *player, int sampleFormat,
				        int sampleRate, int channelFormat);

		} callbacks;

		void *extra;

} player_t;

typedef struct _player_callbacks_t player_callbacks_t;

typedef void (*on_state_change_t)(player_t *player, audio_state_t old_state,
        audio_state_t new_state);

typedef void (*on_play_t)(player_t *player, char *data, int len);

typedef void (*callback_t)(player_t *player);

typedef int (*on_prepare_t)(struct player_t *player, int sampleFormat,
        int sampleRate, int channelFormat);

//one off initialization of the library
int ap_init();

//one off de-initialization of the library
void ap_uninit();

player_t* ap_create(player_callbacks_t callbacks);

void ap_delete(player_t* player);

double ap_get_audio_clock(player_t *player);

int ap_start(player_t *player);

int ap_stop(player_t *player);

int ap_pause(player_t *player);

int ap_set_datasource(player_t *player, const char* url);

int ap_prepare_async(player_t *player);

void ap_reset(player_t *player);

void ap_seek(player_t *player, int64_t pos, int relative);

void ap_print_status(player_t *player);

void ap_print_metadata(player_t *player);

//duration of current track in ms
int32_t ap_get_duration(player_t *player);

//position in current track in ms
int32_t ap_get_position(player_t *player);

int ap_is_playing(player_t *player);

int ap_is_looping(player_t *player);

void ap_set_looping(player_t *player, int looping);

void ap_print_error(const char* msg, int err);

#define BEGIN_LOCK(player) pthread_mutex_lock(&player->mutex)
#define END_LOCK(player) pthread_mutex_unlock(&player->mutex)
#define AP_EVENT(player,event,arg1,arg2) if (player->callbacks.on_event)\
		player->callbacks.on_event(player,event,arg1,arg2)

#endif //_AUDIOPLAYER_H_
