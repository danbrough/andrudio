/*
 * avplay : Simple Media Player based on the Libav libraries
 * Copyright (c) 2003 Fabrice Bellard
 *
 * This file is part of Libav.
 *
 * Libav is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Libav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libav; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <fcntl.h>

#include <libavutil/avstring.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>

#include "audioplayer.h"
#include "packet_queue.h"

//extern int seek_by_bytes;d

const inline char * ap_get_state_name(audio_state_t state) {
	switch (state) {
		case STATE_IDLE:
			return "idle";
		case STATE_INITIALIZED:
			return "initialized";
		case STATE_STOPPED:
			return "stopped";
		case STATE_PAUSED:
			return "paused";
		case STATE_PREPARING:
			return "preparing";
		case STATE_PREPARED:
			return "prepared";
		case STATE_STARTED:
			return "started";
		case STATE_COMPLETED:
			return "completed";
		default:
			return "illegal state";
	}
}

extern void play_thread(player_t *player);

int change_state(player_t *player, audio_state_t state) {
	int ret = -1;
	log_trace("change_state() %s", ap_get_state_name(state));
	BEGIN_LOCK(player);
	audio_state_t old_state = player->state;

	if ((state == STATE_IDLE || state == STATE_ERROR || state == STATE_END)
	        || (old_state == STATE_IDLE && state == STATE_INITIALIZED)

	        || (old_state == STATE_INITIALIZED
	                && (state == STATE_PREPARING || state == STATE_PREPARED))

	        || (old_state == STATE_PREPARING && state == STATE_PREPARED)

	        || (old_state == STATE_PREPARED
	                && (state == STATE_STOPPED || state == STATE_STARTED))

	        || (old_state == STATE_STARTED
	                && (state == STATE_PAUSED || STATE_STOPPED
	                        || STATE_COMPLETED))

	        || (old_state == STATE_PAUSED
	                && (state == STATE_STOPPED || state == STATE_STARTED))

	        || (old_state == STATE_COMPLETED
	                && (state == STATE_STOPPED || state == STATE_STARTED))

	        || (old_state == STATE_STOPPED
	                && (state == STATE_PREPARING || state == STATE_PREPARED))

	        ) {
		ret = SUCCESS;
	}

	if (ret == SUCCESS) {
		player->state = state;
		if (player->callbacks.on_event)
			player->callbacks.on_event(player, EVENT_STATE_CHANGE, old_state,
			        state);
	}
	else {
		log_error("invalid state change: %s -> %s",
		        ap_get_state_name(old_state), ap_get_state_name(state));
	}
	END_LOCK(player);
	return ret;
}

void ap_print_error(const char* msg, int err) {
	char buf[128];
	if (av_strerror(err, buf, sizeof(buf)) == 0) {
		log_error("%s: \"%s\"", msg, buf);
	}
	else {
		log_error("%s unknown error: %d", msg, err);
	}
}

/* get the current audio output buffer size, in samples. With SDL, we
 cannot have a precise information */
static int audio_write_get_buf_size(player_t *player) {
	return player->audio_buf_size - player->audio_buf_index;
}

/* get the current audio clock value */
double ap_get_audio_clock(player_t *player) {
	double pts = 0;
	int hw_buf_size, bytes_per_sec;
	pts = player->audio_clock;

	hw_buf_size = audio_write_get_buf_size(player);

	bytes_per_sec = 0;
	if (player->audio_st) {
		bytes_per_sec = player->sdl_sample_rate * player->sdl_channels
		        * av_get_bytes_per_sample(player->sdl_sample_fmt);
	}

	if (bytes_per_sec)
		pts -= (double) hw_buf_size / bytes_per_sec;
	return pts;
}

/* seek in the stream */
static void stream_seek(player_t *player, int64_t pos, int64_t rel) {
	BEGIN_LOCK(player);
	log_trace("stream_seek %"PRIu64" : %"PRIu64, pos, rel);
	if (!player->seek_req) {
		player->seek_pos = pos;
		player->seek_rel = rel;
		player->seek_flags = AVSEEK_FLAG_FRAME;
		/*	player->seek_flags &= ~AVSEEK_FLAG_BYTE;
		 if (seek_by_bytes)
		 player->seek_flags |= AVSEEK_FLAG_BYTE;*/
		player->seek_req = 1;
	}
	END_LOCK(player);
}

/* pause or resume the video */
int ap_pause(player_t *player) {
	log_trace("ap_pause()");
	BEGIN_LOCK(player);
	int ret = 0;
	if (player->state == STATE_PAUSED) {
		ret = change_state(player, STATE_STARTED);
	}
	else if (player->state == STATE_STARTED) {
		ret = change_state(player, STATE_PAUSED);
	}
	else {
		log_error("ap_pause::invalid state %s",
		        ap_get_state_name(player->state));
	}
	END_LOCK(player);
	return ret;
}

static void log_callback_help(void *ptr, int level, const char *fmt, va_list vl) {
	vfprintf(stdout, fmt, vl);
}

int ap_init() {
	log_debug("ap_init()");

	packet_queue_library_init();

	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_log_set_callback(log_callback_help);
	/* register all codecs, demux and protocols */
	avcodec_register_all();
	/*
	 #if CONFIG_AVDEVICE
	 avdevice_register_all();
	 #endif
	 */

	av_register_all();
	avformat_network_init();

	return 0;
}

void ap_uninit() {
	log_debug("ap_uninit()");
	avformat_network_deinit();
}

player_t* ap_create(player_callbacks_t callbacks) {
	player_t *player = malloc(sizeof(player_t));
	if (!player)
		return NULL;
	memset(player, 0, sizeof(player_t));
	player->callbacks = callbacks;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&player->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	return player;
}

void ap_delete(player_t* player) {
	log_info("ap_delete()");
	if (!player)
		return;

	ap_reset(player);

	pthread_mutex_destroy(&player->mutex);
	free(player);
}

void ap_reset(player_t *player) {
	log_debug("ap_reset()");

	player->abort_request = 1;

//	if (player->play_thread_alive) {
	if (player->play_thread) {
		log_trace("ap_reset::waiting for the play thread");
		pthread_join(player->play_thread, NULL);
		log_trace("ap_reset::play thread done");
	}
	player->play_thread = 0;

	/*
	 }
	 else {
	 log_trace("ap_reset::play thread hasnt been started");
	 }
	 */

	//if (player->read_thread_alive) {
	if (player->read_thread) {
		log_trace("ap_reset::waiting for the read thread..");
		pthread_join(player->read_thread, NULL);
		log_trace("ap_reset::read thread done");
	}
	/*	}
	 else {
	 log_trace("ap_reset::read thread hasnt been started");
	 }*/

	/*
	 #if !CONFIG_AVFILTER
	 if (player->img_convert_ctx)
	 sws_freeContext(player->img_convert_ctx);
	 #endif
	 */

	log_trace("ap_reset::cleaning up..");
	if (player->avr) {
		log_trace("ap_reset::avresample_free()");
		avresample_free(&player->avr);
	}

	if (player->frame) {
		log_trace("ap_reset::av_frame_free()");
		av_frame_free(&player->frame);
	}

	log_trace("ap_reset::freeing packet");
	av_free_packet(&player->audio_pkt);
	//av_free_packet(&player->audio_pkt_temp);
	/*

	 if (player->audio_buf) {
	 log_error("ap_reset::freeing audio buf");
	 av_free(player->audio_buf);
	 player->audio_buf = NULL;
	 }
	 if (player->audio_buf1) {
	 log_error("ap_reset::freeing audio buf1");
	 av_free(player->audio_buf1);
	 player->audio_buf1 = NULL;
	 }
	 */

	if (player->ic) {
		log_trace("ap_reset::avformat_close_input()");
		avformat_close_input(&player->ic);
		avformat_free_context(player->ic);
	}

	end:

	if (player->state != STATE_IDLE) {
		log_trace("ap_reset::changing state to idle");
		change_state(player, STATE_IDLE);
	}
	player->audio_stream = -1;
	player->audio_st = NULL;
	player->abort_request = 0;
	player->avr = NULL;
	player->ic = NULL;
	player->frame = NULL;
	player->url[0] = 0;

	log_trace("ap_reset::done");

}

int ap_set_datasource(player_t *player, const char *url) {
	log_info("ap_set_datasource() %s", url);
	BEGIN_LOCK(player);
	av_strlcpy(player->url, url, sizeof(player->url));
	int ret = change_state(player, STATE_INITIALIZED);
	END_LOCK(player);
	return ret;
}

extern int read_thread(player_t *player);
extern void play_thread(player_t *player);

int ap_prepare_async(player_t *player) {
	int ret = 0;
	log_debug("ap_prepare_async()");

	BEGIN_LOCK(player);
	player->abort_request = 0;
	if ((ret = change_state(player, STATE_PREPARING)) == SUCCESS) {
		log_trace("ap_prepare_async::starting decode thread..");
		if ((ret = pthread_create(&player->read_thread, NULL,
		        (void*) read_thread, player)) != SUCCESS) {
			log_error("failed to start decode thread: %s", strerror(errno));
		}
	}
	END_LOCK(player);

	return ret;
}

void ap_seek(player_t *player, int64_t incr, int relative) {
	log_debug("ap_seek() :%"PRIi64" relative: %s", incr,
	        relative ? "true" : "false");

	BEGIN_LOCK(player);
	if (!player->audio_st)
		goto end;

	int64_t pos;
	if (relative) {
		pos = ap_get_audio_clock(player) * AV_TIME_BASE;
		pos += incr;
	}
	else {
		pos = incr;
	}
	stream_seek(player, pos, incr);

	end:
	END_LOCK(player);
}

/*void ap_seek(player_t *player, int64_t incr) {
 log_debug("ap_seek() :%"PRIi64, incr);

 BEGIN_LOCK(player);
 if (!player->audio_st)
 goto end;
 double pos;
 if (seek_by_bytes) {
 log_trace("seek_by_bytes");
 if (player->audio_stream >= 0 && player->audio_pkt.pos >= 0) {
 pos = player->audio_pkt.pos;
 }
 else
 pos = avio_tell(player->ic->pb);
 if (player->ic->bit_rate)
 incr *= player->ic->bit_rate / 8.0;
 else
 incr *= 180000.0;
 pos += incr;
 stream_seek(player, pos, incr, 1);
 }
 else {
 pos = ap_get_audio_clock(player);
 pos += incr;
 stream_seek(player, (int64_t) (pos * AV_TIME_BASE),
 (int64_t) (incr * AV_TIME_BASE), 0);
 }
 end:
 END_LOCK(player);
 }*/

int ap_start(player_t *player) {
	log_info("ap_start()");
	int ret = 0;
	BEGIN_LOCK(player);
	ret = change_state(player, STATE_STARTED);
	if (ret == 0) {
		pthread_create(&player->play_thread, NULL, (void*) play_thread, player);
	}
	END_LOCK(player);
	return ret;
}

int ap_stop(player_t *player) {
	log_info("ap_stop()");
	player->abort_request = 1;
	return change_state(player, STATE_STOPPED);
}

void ap_print_status(player_t *player) {
	int hours, mins, secs;

	BEGIN_LOCK(player);

	if (!player->audio_st) {
		log_trace("ap_print_status():: not playing");
		goto end;
	}

	int64_t duration = player->ic->duration;
	if (duration == AV_NOPTS_VALUE) {
		duration = -1;
		hours = mins = secs = 0;
	}
	else {
		duration = duration / AV_TIME_BASE;
		secs = duration % 60;
		hours = (duration / (60 * 60));
		mins = (duration - hours * 60 * 60) / 60;
	}
	log_trace(
	        "ap_print_status(): state:%s pos:%.2f duration:%"PRIi64" %02d:%02d:%02d",
	        ap_get_state_name(player->state), ap_get_audio_clock(player),
	        player->ic->duration == AV_NOPTS_VALUE ? 0 : player->ic->duration,
	        hours, mins, secs);
	end:
	END_LOCK(player);
}

void ap_print_metadata(player_t *player) {
	if (player->ic) {
		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(player->ic->metadata, "", entry,
		AV_DICT_IGNORE_SUFFIX)))
			log_trace("metadata:\t%s:%s", entry->key, entry->value);
	}
}

//duration of current track in ms
int32_t ap_get_duration(player_t *player) {

	if (player->state == STATE_PREPARED || player->state == STATE_STARTED
	        || player->state == STATE_PAUSED || player->state == STATE_STOPPED
	        || player->state == STATE_COMPLETED) {
		if (player && player->ic
		        && player->ic->duration
		                > 0&& player->ic->duration != AV_NOPTS_VALUE)
			return (int32_t) (player->ic->duration / 1000);
	}
	else {
		log_error("ap_get_duration() called in illegal state: %s",
		        ap_get_state_name(player->state));
	}
	return -1;
}

//position in current track in ms
int32_t ap_get_position(player_t *player) {
	if (player->state == STATE_ERROR) {
		log_error("ap_get_position called in illegal state");
		return 0;
	}


	double pos = ap_get_audio_clock(player);
	log_trace("ap_get_position:: clock %f",pos);

	return (int32_t) (ap_get_audio_clock(player) * 1000);
}

int ap_is_playing(player_t *player) {
	return player->state == STATE_STARTED;
}

int ap_is_looping(player_t *player) {
	return player->looping;
}

void ap_set_looping(player_t *player, int looping) {
	player->looping = looping;
}
