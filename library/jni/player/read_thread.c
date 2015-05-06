#include <assert.h>

#include "audioplayer.h"
#include <pthread.h>

static int decode_interrupt_cb(player_t *player) {
	return player && player->abort_request;
}

//see audiostream.c
// "seek by bytes 0=off 1=on -1=auto"
//int seek_by_bytes = -1;
extern AVPacket flush_pkt;
static int workaround_bugs = 1;
//"non spec compliant optimizations"
static int fast = 0;
const int genpts = 0;
static int idct = FF_IDCT_AUTO;
static enum AVDiscard skip_idct = AVDISCARD_DEFAULT;
static enum AVDiscard skip_loop_filter = AVDISCARD_DEFAULT;
static int error_concealment = 3;
//  "don't limit the input buffer size (useful with realtime streams)"
static int infinite_buffer = 0;

static int wanted_stream[AVMEDIA_TYPE_NB] = { [AVMEDIA_TYPE_AUDIO] = -1,
        [AVMEDIA_TYPE_VIDEO] = -1, [AVMEDIA_TYPE_SUBTITLE] = -1, };

extern int change_state(player_t *player, audio_state_t state);

/* open a given stream. Return 0 if OK */
static int stream_component_open(player_t *player, int stream_index) {
	AVFormatContext *ic = player->ic;
	AVCodecContext *avctx;
	AVCodec *codec;

	AVDictionary *opts = NULL;
	AVDictionaryEntry *t = NULL;
	int ret = 0;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return -1;
	avctx = ic->streams[stream_index]->codec;

	/*	opts = filter_codec_opts(codec_opts, avctx->codec_id, ic,
	 ic->streams[stream_index], NULL);*/

	codec = avcodec_find_decoder(avctx->codec_id);
	if (!codec)
		goto end;

	avctx->workaround_bugs = workaround_bugs;
	avctx->idct_algo = idct;
	avctx->skip_idct = skip_idct;
	avctx->skip_loop_filter = skip_loop_filter;
	avctx->error_concealment = error_concealment;

	if (fast)
		avctx->flags2 |= CODEC_FLAG2_FAST;

	if (!av_dict_get(opts, "threads", NULL, 0)) {
		av_dict_set(&opts, "threads", "auto", 0);
	}

	if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		ap_print_error("avcodec_open2() failed", ret);
		goto end;
	}

	if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto end;
	}

	/* prepare audio output */
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		player->sdl_sample_rate = avctx->sample_rate;

		if (!avctx->channel_layout)
			avctx->channel_layout = av_get_default_channel_layout(
			        avctx->channels);

		if (!avctx->channel_layout) {
			log_error("unable to guess channel layout");
			ret = AVERROR_INVALIDDATA;
			goto end;
		}

		if (avctx->channels == 1)
			player->sdl_channel_layout = AV_CH_LAYOUT_MONO;
		else
			player->sdl_channel_layout = AV_CH_LAYOUT_STEREO;

		player->sdl_channels = av_get_channel_layout_nb_channels(
		        player->sdl_channel_layout);

		int sampleRate = player->sdl_sample_rate;
		int channelFormat = player->sdl_channels;

		if (player->callbacks.on_prepare(player, AV_SAMPLE_FMT_S16, sampleRate,
		        channelFormat) < 0) {
			log_error("on_prepare() failed");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		player->sdl_sample_fmt = AV_SAMPLE_FMT_S16;
		player->resample_sample_fmt = player->sdl_sample_fmt;
		player->resample_channel_layout = avctx->channel_layout;
		player->resample_sample_rate = player->sdl_sample_rate;
		log_trace("stream_component_open::resample_sample_rate: %d",
		        player->sdl_sample_rate);
	}

	ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;

	player->audio_stream = stream_index;
	player->audio_st = ic->streams[stream_index];
	player->audio_buf_size = 0;
	player->audio_buf_index = 0;

	memset(&player->audio_pkt, 0, sizeof(player->audio_pkt));
	packet_queue_init(&player->audioq);

	end: av_dict_free(&opts);
	if (ret == 0)
		change_state(player, STATE_PREPARED);
	return ret;
}

static void stream_component_close(player_t *player, int stream_index) {
	log_debug("stream_component_close() index:%d", stream_index);
	AVFormatContext *ic = player->ic;
	AVCodecContext *avctx;
	if (stream_index == -1)
		return;
	BEGIN_LOCK(player);
	/*if (stream_index < 0 || stream_index >= ic->nb_streams)
	 goto end;*/
	avctx = ic->streams[stream_index]->codec;

	packet_queue_abort(&player->audioq);

	packet_queue_end(&player->audioq);
	av_free_packet(&player->audio_pkt);
	if (player->avr) {
		avresample_free(&player->avr);
		player->avr = NULL;
	}
	av_freep(&player->audio_buf1);
	player->audio_buf = NULL;
	av_frame_free(&player->frame);

	ic->streams[stream_index]->discard = AVDISCARD_ALL;
	avcodec_close(avctx);

	//end: ;
	END_LOCK(player);
	log_debug("stream_component_close::done");

}

/* this thread gets the stream from the disk or the network */
int read_thread(player_t *player) {

	log_debug("read_thread()");
	AVFormatContext *ic = NULL;
	int err, i, ret;
	int st_index[AVMEDIA_TYPE_NB] = { 0 };
	AVPacket pkt1, *pkt = &pkt1;
	int eof = 0;

	AP_EVENT(player, EVENT_THREAD_START, 0, 0);

	memset(st_index, -1, sizeof(st_index));

	player->audio_stream = -1;

	log_trace("read_thread::avformat_alloc_context()");
	ic = avformat_alloc_context();
	if (!ic) {
		av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
		ret = AVERROR(ENOMEM);
		goto fail;
	}

	ic->interrupt_callback.opaque = player;
	ic->interrupt_callback.callback = (void*) decode_interrupt_cb;

	log_trace("read_thread::avformat_open_input() %s", player->url);
	err = avformat_open_input(&ic, player->url, NULL, NULL);
	if (err < 0) {
		ap_print_error("read_thread::avformat_open_input failed", err);
		ret = -1;
		goto fail;
	}

	player->ic = ic;

	if (genpts)
		ic->flags |= AVFMT_FLAG_GENPTS;

	err = avformat_find_stream_info(ic, NULL);
	if (err < 0) {
		ap_print_error("read_thread::avformat_find_stream_info failed", err);
		ret = err;
		goto fail;
	}

	if (ic->pb)
		ic->pb->eof_reached = 0;

	player->audio_stream = -1;

	for (i = 0; i < ic->nb_streams; i++)
		ic->streams[i]->discard = AVDISCARD_ALL;

	st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
	        wanted_stream[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_VIDEO],
	        NULL, 0);

	/* open the streams */
	if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		stream_component_open(player, st_index[AVMEDIA_TYPE_AUDIO]);
	}

	/*	ret = -1;
	 if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
	 ret = stream_component_open(player, st_index[AVMEDIA_TYPE_VIDEO]);
	 }

	 if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
	 stream_component_open(player, st_index[AVMEDIA_TYPE_SUBTITLE]);
	 }*/

	if (player->audio_stream < 0) {
		log_error("read_thread::%s: could not open codecs", player->url);
		ret = -1;
		goto fail;
	}

	log_trace("read_thread:: stream opened .. reading metadata..");

	av_dump_format(ic, 0, player->url, 0);
	AVDictionaryEntry *entry = NULL;
	while ((entry = av_dict_get(ic->metadata, "", entry,
	AV_DICT_IGNORE_SUFFIX)))
		log_trace("metadata:\t%s:%s", entry->key, entry->value);

	log_trace("read_thread::beginning read loop");
	while (!player->abort_request) {
		//log_trace("read_thread::loop");
		int is_paused = player->state == STATE_PAUSED;

		if (is_paused)
			goto sleep;

		if (is_paused != player->last_paused) {
			player->last_paused = is_paused;
			if (is_paused) {
				av_read_pause(ic);
				log_warn("av_read_pause(ic); done");
			}
			else {
				av_read_play(ic);
				log_warn("av_read_play(ic); done");
			}
		}

		if (player->seek_req) {
			log_trace("read_thread::seek_req");
			int64_t seek_target = player->seek_pos;
			int64_t seek_min =
			        player->seek_rel > 0 ? seek_target - player->seek_rel + 2 :
			        INT64_MIN;
			int64_t seek_max =
			        player->seek_rel < 0 ? seek_target - player->seek_rel - 2 :
			        INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

			log_trace("read_thread::avformat_seek_file()");
			ret = avformat_seek_file(player->ic, -1, seek_min, seek_target,
			        seek_max, player->seek_flags);
			if (ret < 0) {
				ap_print_error("read_thread::error in seek", ret);
			}
			else {
				if (player->audio_stream >= 0) {
					packet_queue_flush(&player->audioq);
					packet_queue_put(&player->audioq, &flush_pkt);
				}
			}
			if (player->callbacks.on_event)
				player->callbacks.on_event(player, EVENT_SEEK_COMPLETE, 0, 0);
			player->seek_req = 0;
			eof = 0;
		}

		/* if the queue are full, no need to read more */
		//if (!infinite_buffer
		//  && (player->audioq.size + player->videoq.size + player->subtitleq.size
		if (!infinite_buffer
		        && (player->audioq.size > MAX_QUEUE_SIZE
		                || ((player->audioq.size > MIN_AUDIOQ_SIZE
		                        || player->audio_stream < 0)))) {
			/* wait 10 ms */
			goto sleep;
		}

		if (eof) {
			//log_trace("read_thread::eof");
			if (player->audio_stream >= 0
			        && (player->audio_st->codec->codec->capabilities
			                & CODEC_CAP_DELAY)) {
				av_init_packet(pkt);
				pkt->data = NULL;
				pkt->size = 0;
				pkt->stream_index = player->audio_stream;
				packet_queue_put(&player->audioq, pkt);
			}

			if (player->audioq.size == 0) {
				log_trace("read_thread::player->audioq.size == 0");
				if (player->looping) {
					log_trace("read_thread::looping");
					ap_seek(player, 0, 0);
					continue;
				}
				log_trace("going to fail with AVERROR_EOF");
				ret = AVERROR_EOF;
				goto fail;
			}
			goto sleep;
		}

		ret = av_read_frame(ic, pkt);

		if (ret < 0) {
			ap_print_error("av_read_frame failed", ret);
			if (ret == AVERROR_EOF || (ic->pb && ic->pb->eof_reached)){
				log_trace("eof == 1");
				eof = 1;
			}
			if (ic->pb && ic->pb->error)
				break;
			goto sleep;
		}

		if (pkt->stream_index == player->audio_stream) {
			packet_queue_put(&player->audioq, pkt);
		}
		else {
			av_free_packet(pkt);
		}

		continue;
		sleep: usleep(1000);
	}

	ret = 0;
	fail:
	log_trace("read_loop::finished loop eof: %d", eof);

	stream_component_close(player, player->audio_stream);

	BEGIN_LOCK(player);
	if (player->state == STATE_STARTED)
		change_state(player, STATE_COMPLETED);
	END_LOCK(player);

	log_trace("read_thread::done returning %d", ret);

	return ret;
}

