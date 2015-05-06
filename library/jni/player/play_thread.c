/*
 * playthread.c
 *
 *  Created on: 15/04/2015
 *      Author: dan
 */

#include <libavutil/opt.h>
#include <assert.h>

#include "audioplayer.h"

//packet_queue.c
extern AVPacket flush_pkt;

/* decode one audio frame and returns its uncompressed size */
static int audio_decode_frame(player_t *player, double *pts_ptr) {
	AVPacket *pkt_temp = &player->audio_pkt_temp;
	AVPacket *pkt = &player->audio_pkt;
	AVCodecContext *dec = player->audio_st->codec;
	int n, len1, data_size, got_frame;
	double pts;
	int new_packet = 0;
	int flush_complete = 0;

	for (;;) {
		/* NOTE: the audio packet can contain several frames */
		while (pkt_temp->size > 0 || (!pkt_temp->data && new_packet)) {
			int resample_changed, audio_resample;

			if (!player->frame) {
				if (!(player->frame = av_frame_alloc()))
					return AVERROR(ENOMEM);
			}

			if (flush_complete)
				break;

			new_packet = 0;
			len1 = avcodec_decode_audio4(dec, player->frame, &got_frame,
			        pkt_temp);
			if (len1 < 0) {
				/* if error, we skip the frame */
				pkt_temp->size = 0;
				break;
			}

			pkt_temp->data += len1;
			pkt_temp->size -= len1;

			if (!got_frame) {
				/* stop sending empty packets if the decoder is finished */
				if (!pkt_temp->data
				        && (dec->codec->capabilities & CODEC_CAP_DELAY))
					flush_complete = 1;
				continue;
			}
			data_size = av_samples_get_buffer_size(NULL, dec->channels,
			        player->frame->nb_samples, player->frame->format, 1);

			audio_resample = player->frame->format != player->sdl_sample_fmt
			        || player->frame->channel_layout
			                != player->sdl_channel_layout
			        || player->frame->sample_rate != player->sdl_sample_rate;

			resample_changed = player->frame->format
			        != player->resample_sample_fmt
			        || player->frame->channel_layout
			                != player->resample_channel_layout
			        || player->frame->sample_rate
			                != player->resample_sample_rate;

			if ((!player->avr && audio_resample) || resample_changed) {
				int ret;
				if (player->avr)
					avresample_close(player->avr);
				else if (audio_resample) {
					player->avr = avresample_alloc_context();
					if (!player->avr) {
						fprintf(stderr,
						        "error allocating AVAudioResampleContext\n");
						break;
					}
				}
				if (audio_resample) {
					av_opt_set_int(player->avr, "in_channel_layout",
					        player->frame->channel_layout, 0);
					av_opt_set_int(player->avr, "in_sample_fmt",
					        player->frame->format, 0);
					av_opt_set_int(player->avr, "in_sample_rate",
					        player->frame->sample_rate, 0);
					av_opt_set_int(player->avr, "out_channel_layout",
					        player->sdl_channel_layout, 0);
					av_opt_set_int(player->avr, "out_sample_fmt",
					        player->sdl_sample_fmt, 0);
					av_opt_set_int(player->avr, "out_sample_rate",
					        player->sdl_sample_rate, 0);

					if ((ret = avresample_open(player->avr)) < 0) {
						fprintf(stderr, "error initializing libavresample\n");
						break;
					}
				}
				player->resample_sample_fmt = player->frame->format;
				player->resample_channel_layout = player->frame->channel_layout;
				player->resample_sample_rate = player->frame->sample_rate;
			}

			if (audio_resample) {
				void *tmp_out;
				int out_samples, out_size, out_linesize;
				int osize = av_get_bytes_per_sample(player->sdl_sample_fmt);
				int nb_samples = player->frame->nb_samples;

				out_size = av_samples_get_buffer_size(&out_linesize,
				        player->sdl_channels, nb_samples,
				        player->sdl_sample_fmt, 0);
				tmp_out = av_realloc(player->audio_buf1, out_size);
				if (!tmp_out)
					return AVERROR(ENOMEM);
				player->audio_buf1 = tmp_out;

				out_samples = avresample_convert(player->avr,
				        &player->audio_buf1, out_linesize, nb_samples,
				        player->frame->data, player->frame->linesize[0],
				        player->frame->nb_samples);
				if (out_samples < 0) {
					ap_print_error("avresample_convert() failed", out_samples);
					break;
				}
				player->audio_buf = player->audio_buf1;
				data_size = out_samples * osize * player->sdl_channels;
			}
			else {
				player->audio_buf = player->frame->data[0];
			}

			/* if no pts, then compute it */
			/*pts = player->audio_clock;
			*pts_ptr = pts;*/
			n = player->sdl_channels
			        * av_get_bytes_per_sample(player->sdl_sample_fmt);
			player->audio_clock += (double) data_size
			        / (double) (n * player->sdl_sample_rate);

			if (pkt->pts != AV_NOPTS_VALUE) {
				player->audio_clock = av_q2d(player->audio_st->time_base)
				        * pkt->pts;
			}


#ifdef DEBUG
			{
				static double last_clock;
				printf("audio: delay=%0.3f clock=%0.3f pts=%0.3f\n",
						player->audio_clock - last_clock,
						player->audio_clock, pts);
				last_clock = player->audio_clock;
			}
#endif
			return data_size;
		}

		/* free the current packet */
		if (pkt->data)
			av_free_packet(pkt);
		memset(pkt_temp, 0, sizeof(*pkt_temp));

		if (player->state == STATE_PAUSED || player->audioq.abort_request
		        || player->abort_request) {
			log_trace("audio_decode_frame::exiting");
			return -1;
		}

		/* read next packet */
		if ((new_packet = packet_queue_get(&player->audioq, pkt, 1)) < 0)
			return -1;

		if (pkt->data == flush_pkt.data) {
			avcodec_flush_buffers(dec);
			flush_complete = 0;
		}

		*pkt_temp = *pkt;

		/* if update the audio clock with the pts */
		if (pkt->pts != AV_NOPTS_VALUE) {
			player->audio_clock = av_q2d(player->audio_st->time_base)
			        * pkt->pts;
		}
	}
}

/* prepare a new audio buffer */
void play_thread(player_t *player) {
	log_info("play_thread()");
	assert(player);
//pthread_cleanup_push(play_thread_cleanup,player);
	int audio_size, len1;
	double pts;

	AP_EVENT(player, EVENT_THREAD_START, 0, 0);
	//audio_callback_time = av_gettime();
	//while (player->state == STATE_PLAYING) {
	while (!player->abort_request
	        && (player->state == STATE_STARTED || player->state == STATE_PAUSED)) {
		int len = SDL_AUDIO_BUFFER_SIZE;
		if (player->state != STATE_STARTED) {
			usleep(1000);
			continue;
		}
		while (len > 0
		        && (player->state == STATE_STARTED
		                || player->state == STATE_PAUSED)
		        && !player->abort_request) {
			if (player->audio_buf_index >= player->audio_buf_size) {
				audio_size = audio_decode_frame(player, &pts);
				if (audio_size < 0) {
					/* if error, just output silence */
					player->audio_buf = player->silence_buf;
					player->audio_buf_size = sizeof(player->silence_buf);
				}
				else {
					player->audio_buf_size = audio_size;
				}
				player->audio_buf_index = 0;
			}
			len1 = player->audio_buf_size - player->audio_buf_index;
			if (len1 > len)
				len1 = len;
			//memcpy(stream, (uint8_t *) player->audio_buf + player->audio_buf_index, len1);
			//log_trace("sdl_audio_callback::playing %d", len1);

			if (player->abort_request)
				continue;

			//log_trace("play_thread::locking player");
			//BEGIN_LOCK(player);
			player->callbacks.on_play(player,
			        (char*) player->audio_buf + player->audio_buf_index, len1);
			//END_LOCK(player);
			//log_trace("play_thread::player unlocked");

			len -= len1;
			//stream += len1;
			player->audio_buf_index += len1;
		}
	}

	log_trace("play_thread::done");
	//pthread_exit(0);

}
