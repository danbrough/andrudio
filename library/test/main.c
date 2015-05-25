#include <signal.h>

#include <termios.h>

#include "audioplayer.h"

#ifndef DISABLE_AUDIO
#define USING_AO
#endif

#ifdef USING_AO
#include <ao/ao.h>
static int driver = -1;
static ao_device* device = NULL;
#endif

static int seek_minute = 1;
static int seek_relative = 0;

static const char* url =
		"http://www.audiocheck.net/Audio/audiocheck.net_putyourhands.mp3";

static int on_prepare(player_t *player, int sampleFormat, int sampleRate,
		int channelFormat) {
	log_debug("on_prepare()");

#ifdef USING_AO
	if (device)
		ao_close(device);

	log_debug("on_prepare::opening channels:%d rate:%d", channelFormat,
			sampleRate);
	log_trace("freq : %d", sampleRate);
	ao_sample_format sample_format;

	sample_format.bits = 16;
	sample_format.channels = channelFormat;
	//sample_format.rate = spec->freq;
	sample_format.rate = sampleRate;
	sample_format.byte_format = AO_FMT_NATIVE;
	sample_format.matrix = 0;

	device = ao_open_live(driver, &sample_format, NULL);
#endif

	log_debug("on_prepare::done");
	return 0;
}

static void nonblock(int state) {
	struct termios ttystate;

	//get the terminal state
	tcgetattr(STDIN_FILENO, &ttystate);

	if (state == 1) {
		//turn off canonical mode
		ttystate.c_lflag &= ~ICANON;
		ttystate.c_lflag &= ~ECHO;
		//minimum of number input read.
		ttystate.c_cc[VMIN] = 1;
	} else if (state == 0) {
		//turn on canonical mode
		ttystate.c_lflag |= ICANON;
		ttystate.c_lflag |= ECHO;
	}

	//set the terminal attributes.
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

static void on_play(player_t *player, char *data, int len) {
#ifdef USING_AO
	ao_play(device, data, len);
#endif
}

static void on_event(player_t *player, audio_event_t event, int arg1, int arg2) {

	audio_state_t old_state = arg1;
	audio_state_t state = arg2;

	switch (event) {
	case EVENT_THREAD_START:
	case EVENT_SEEK_COMPLETE:
		break;

	case EVENT_STATE_CHANGE:
		log_trace("on_event::STATE_CHANGE() %s->%s",
				ap_get_state_name(old_state), ap_get_state_name(state));
		if (state == STATE_PREPARED) {
			log_warn(
					"on_state_change::STATE_PREPARED: starting the stream automatically");
			ap_start(player);
		} else if (state == STATE_STARTED) {
			log_trace("on_state_change::(hit 'd' to stop or space to pause)");
		} else if (state == STATE_COMPLETED) {
			log_trace("on_state_change::COMPLETED");
		}
		break;
	}
}

static void play(player_t *player, const char *url) {
	log_info("play() %s", url);
	seek_minute = 1;
	ap_reset(player);
	ap_set_datasource(player, url);
	ap_prepare_async(player);
}

static player_t *global_player = NULL;

static void do_exit(void) {
	log_warn("do_exit()");
	nonblock(0);

	player_t *player = global_player;
	if (player) {
		ap_delete(player);
	}

#ifdef USING_AO
	if (device) {
		log_trace("do_exit::closing device");
		ao_close(device);
		device = NULL;
	}
	log_trace("ao_shutdown()");
	ao_shutdown();
#endif

	ap_uninit();

	log_debug("do_exit::done.");
	pthread_exit(0);
	exit(0);
}

#include <sys/epoll.h>

static void event_loop(player_t *player) {

	nonblock(1);

	int c = 0;

	double incr;
	int printed_not_playing = 0;
	//FD_ZERO(&rfds);
	//FD_SET(STDIN_FILENO, &rfds);
	const int MAX_EVENTS = 64;
	struct epoll_event event;
	struct epoll_event events[64];
	int efd = epoll_create1(0);

	memset(&events, 0, sizeof(events));

	if (efd == -1) {
		log_error("epoll_create1() failed");
		return;
	}

	memset(&event, 0, sizeof(struct epoll_event));
	event.events = EPOLLIN|EPOLLOUT;
	event.data.fd = STDIN_FILENO;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0) {
		log_error("epoll set insertion error: fd=%d0", STDIN_FILENO);
		return;
	}

	while (1) {

		c = -1;
		log_warn("epoll_wait()");
		int nfds = epoll_wait(efd, events, MAX_EVENTS, 1000);
		//int ret = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
		if (nfds < 0) {
			log_error("epoll_wait failed: error:%d:%s", errno, strerror(errno));
			do_exit();
		}
		log_trace("select returned %d", nfds);

		for (int n = 0; n < nfds; n++) {
			if (events[n].data.fd == STDIN_FILENO) {
				log_warn("stdin event events: %d",event.events);
				read(STDIN_FILENO, &c, 1);
				break;
			}
		}

		if (nfds == 0) {
			//timeout occurred. Print the status
			c = 'p';
		}

		/*	if (c == -1 || (!player->audio_st && ((char) c) != 'q'))
		 continue;
		 */

		switch ((char) c) {
		case '1':
			play(player, url);
			break;
		case '2':
			play(player, "mmsh://streaming.radionz.co.nz/national-mbr");
			break;
		case '3':
			play(player,
					"rtsp://radionz-wowza.streamguys.com/national/national.stream");
			break;
		case '4':
			play(player, "http://radionz-ice.streamguys.com/national.mp3");
			break;
		case '5':
			play(player, "http://stream.radioactive.fm:8000/ractive");
			break;
		case '6':
			play(player, "http://live-aacplus-64.kexp.org/kexp64.aac");
			break;
		case '7':
			play(player, "./test48.ogg");
			break;
		case '8':
			play(player, "./test.mp3");
			break;
		case '9':
			play(player, "./test.ogg");
			break;
		case '0':
			printf("no more tests\n");
			break;
		case 'q':
			log_trace("read a quit!");
			do_exit();
			break;
		case ' ':
			log_trace("toggle_pause()");
			ap_pause(player);
			break;
		case 'p':
			if (!player->audio_st) {
				if (!printed_not_playing) {
					ap_print_status(player);
					printed_not_playing = 1;
				}
			} else {
				printed_not_playing = 0;
				ap_print_status(player);
			}
			break;
		case 's':
			ap_start(player);
			break;
		case 'd':
			ap_stop(player);
			break;
		case 'r':
			ap_reset(player);
			break;
		case 'z':
			ap_seek(player, 0, FALSE);
			break;
		case 'm':
			ap_print_metadata(player);
			break;
		case 'l':
			player->looping = !player->looping;
			log_info("player->looping == %s",
					(player->looping ? "true" : "false"));
			break;
		case 'o':
			ap_seek(player, 60000, 0);
			break;
		case 0x1b:
			read(STDIN_FILENO, &c, 1);
			if ((char) c == 0x5b) {
				read(STDIN_FILENO, &c, 1);

				switch ((char) c) {
				case 'D':
					incr = -10.0;
					seek_relative = TRUE;
					goto do_seek;
				case 'C':
					incr = 10.0;
					seek_relative = TRUE;
					goto do_seek;
				case 'A':
					incr = 60.0 * seek_minute;
					seek_minute++;
					log_debug("seeking to minute: %d", seek_minute);
					seek_relative = FALSE;
					goto do_seek;
				case 'B':
					seek_minute--;
					if (seek_minute == 0)
						seek_minute = 1;
					incr = 60.0 * seek_minute;
					log_debug("seeking to minute: %d", seek_minute);
					seek_relative = FALSE;

					do_seek: ap_seek(player, incr * AV_TIME_BASE,
							seek_relative);
					ap_print_status(player);

					break;
				}
			}
			break;

		default:
			log_error("invalid key: %"PRIu16 ":%c", c, c);
			log_info("q:\tquit");
			log_info("space:\tpause");
			log_info("s:\tstart");
			log_info("d:\tstop");
			log_info("r:\treset");
			log_info("p:\tstatus");
			log_info("z:\tseek to start");
			log_info("m:\tprint metadata");
			log_info("o:\tseek to one minute");
			log_info("l:\ttoggle loop mode");
			log_info("right arrow:\tseek +10 seconds");
			log_info("left arrow:\tseek -10 seconds");
			log_info(
					"up arrow:\tabsolute seek to the first minute,2nd minute, ...");
			log_info("down arrow:\tabsolute seek to the previous minute");
			log_info("1,2,3,4,5..:\tplay a test track");
			break;
		}
	}

}

static void exit_handler(int signal) {
	do_exit();
}

int main(int argc, char **argv) {

#ifdef USING_AO
	ao_initialize();
	driver = ao_default_driver_id();
#endif

	ap_init();

	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = exit_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	if (argc > 1)
		url = argv[1];
	//pipe(cmd_pipe);

	player_callbacks_t callbacks;
	memset(&callbacks, 0, sizeof(player_callbacks_t));
	callbacks.on_play = on_play;
	callbacks.on_prepare = on_prepare;
	callbacks.on_event = on_event;

	player_t* player = ap_create(callbacks);

	global_player = player;
	ap_set_datasource(player, url);
	ap_prepare_async(player);

	event_loop(player);

	/* never returns */
	return 0;
}
