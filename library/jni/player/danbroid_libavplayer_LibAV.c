#include "audioplayer.h"
#include "danbroid_libavplayer_LibAV.h"
#include <assert.h>

#define JLONG_TO_PLAYER(handle) (player_t*)(intptr_t) handle
#define PLAYER_TO_JLONG(stream) (jlong)(intptr_t) stream

typedef struct fields_t {
	jclass class_audio_stream;
	jmethodID onPrepared;
	jmethodID writeAudio;
	jmethodID onStateChanged;
	jmethodID handleEvent;
} fields_t;

typedef struct _JavaInfo {
	jobject listener;
	jbyteArray buffer;
	int size;
} JavaInfo;

static fields_t fields;
static JavaVM *jvm;
static pthread_key_t current_jni_env;

static void detach_current_thread(void *env) {
	log_info("detach_current_thread() %"PRIX32, (uint32_t ) pthread_self());
	(*jvm)->DetachCurrentThread(jvm);
}
static JNIEnv* attach_current_thread() {
	log_info("attach_current_thread() %"PRIX32, (uint32_t ) pthread_self());
	JNIEnv *env = 0;
	int ret = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if (ret < 0) {
		log_error("error attaching thread %d:%s", ret, strerror(ret));
		return NULL;
	}
	return env;
}

static JNIEnv *get_jni_env(void) {
	JNIEnv *env;
	if ((env = pthread_getspecific(current_jni_env)) == NULL) {
		env = attach_current_thread();
		pthread_setspecific(current_jni_env, env);
	}
	return env;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *pvt) {
	log_info("JNI_OnLoad()");
	jvm = vm;
	pthread_key_create(&current_jni_env, detach_current_thread);
	return JNI_VERSION_1_6;
}

void jniThrowException(JNIEnv* env, const char* className, const char* msg) {
	jclass exception = (*env)->FindClass(env, className);
	(*env)->ThrowNew(env, exception, msg);
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_initialiseLibrary(
        JNIEnv *env, jclass jCls, jclass listenerCls) {
	log_info("Java_danbroid_libavplayer_LibAV_initialise()");

	fields.class_audio_stream = listenerCls;
	assert(fields.class_audio_stream);

	fields.writeAudio = (*env)->GetMethodID(env, listenerCls, "writeAudio",
	        "([BII)V");

	assert(fields.writeAudio);

	fields.onPrepared = (*env)->GetMethodID(env, listenerCls, "onPrepared",
	        "(III)V");
	assert(fields.onPrepared);

	fields.handleEvent = (*env)->GetMethodID(env, listenerCls, "handleEvent",
	        "(III)V");
	assert(fields.handleEvent);

	return ap_init();
}

static void callback_on_play(player_t *player, char *data, int len) {
	///log_trace("callback_on_play() %d", len);

	JavaInfo *info = (JavaInfo*) player->extra;

	JNIEnv *env = get_jni_env();
	if (info->size < len) {
		log_debug("old buffer too small");
		(*env)->DeleteGlobalRef(env, info->buffer);
		info->buffer = NULL;
	}

	if (!info->buffer) {
		info->size = len;
		log_debug("created new buffer of size: %d", info->size);
		info->buffer = (*env)->NewByteArray(env, info->size);
		info->buffer = (*env)->NewGlobalRef(env, info->buffer);
	}

	(*env)->SetByteArrayRegion(env, info->buffer, 0, len, (jbyte*) data);

	if (info->listener) {
		(*env)->CallVoidMethod(env, info->listener, fields.writeAudio,
		        info->buffer, 0, len);
	}
}

static int callback_on_prepare(player_t *player, SDL_AudioSpec *wanted,
        SDL_AudioSpec *spec) {
	log_trace("callback_on_prepare()");
	JNIEnv *env = get_jni_env();
	memccpy(spec, wanted, 1, sizeof(SDL_AudioSpec));

	JavaInfo *info = (JavaInfo*) player->extra;
	(*env)->CallVoidMethod(env, info->listener, fields.onPrepared,
	        AV_SAMPLE_FMT_S16, spec->freq, spec->channels);

	return 0;
}

static void callback_on_event(struct player_t *player, audio_event_t event,
        int arg1, int arg2) {
	JavaInfo *info = (JavaInfo*) player->extra;
	JNIEnv *env = 0;

	switch (event) {
		case EVENT_THREAD_START:
			//nothing to do here
			log_trace("callback_on_event::EVENT_THREAD_START %"PRIX32,
			        (uint32_t )pthread_self());
			break;
		default:
			assert(info);
			assert(info->listener);
			env = get_jni_env();
			(*env)->CallVoidMethod(env, info->listener, fields.handleEvent,
			        event, arg1, arg2);
			break;
	}
}

JNIEXPORT jlong JNICALL Java_danbroid_libavplayer_LibAV_create(JNIEnv *env,
        jclass jCls) {
	log_info("Java_danbroid_libavplayer_LibAV_create()");
	player_callbacks_t callbacks;
	memset(&callbacks, 0, sizeof(player_callbacks_t));

	callbacks.on_play = callback_on_play;
	callbacks.on_prepare = callback_on_prepare;
	callbacks.on_event = callback_on_event;

	player_t* audio = ap_create(callbacks);

	if (!audio) {
		log_error("failed to create player");
		return 0;
	}
	JavaInfo *extra = malloc(sizeof(JavaInfo));
	memset(extra, 0, sizeof(JavaInfo));
	audio->extra = extra;

	return PLAYER_TO_JLONG(audio);
}
JNIEXPORT void JNICALL Java_danbroid_libavplayer_LibAV_destroy(JNIEnv *env,
        jclass jCls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	log_info("Java_danbroid_libavplayer_LibAV_destroy()");

	JavaInfo *info = (JavaInfo*) player->extra;

	ap_delete(player);

	jbyteArray buf = info->buffer;
	if (info) {
		if (info->buffer) {
			log_trace(
			        "Java_danbroid_libavplayer_LibAV_destroy::(*env)->DeleteGlobalRef(env, info->buffer);");
			(*env)->DeleteGlobalRef(env, info->buffer);
		}
		info->buffer = NULL;

		if (info->listener) {
			log_trace(
			        "Java_danbroid_libavplayer_LibAV_destroy::(*env)->DeleteGlobalRef(env, info->listener);");
			(*env)->DeleteGlobalRef(env, info->listener);
		}
	}

	log_trace("Java_danbroid_libavplayer_LibAV_destroy::free(info)");
	free(info);
	log_trace("Java_danbroid_libavplayer_LibAV_destroy::done");

}
JNIEXPORT void JNICALL Java_danbroid_libavplayer_LibAV_setListener(JNIEnv *env,
        jclass cls, jlong handle, jobject listener) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	JavaInfo *info = (JavaInfo*) player->extra;
	if (info->listener) {
		(*env)->DeleteGlobalRef(env, info->listener);
		info->listener = NULL;
	}

	if (listener) {
		info->listener = (*env)->NewGlobalRef(env, listener);
	}
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_stop(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}
	return ap_stop(player);
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_reset(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	ap_reset(player);
	return 0;
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_start(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_start(player);
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_togglePause(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return (jint) ap_pause(player);
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_getDuration(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_get_duration(player);
}

/*
 * Class:     danbroid_libavplayer_LibAV
 * Method:    getPosition
 * Signature: (J)J
 */
JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_getPosition(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}
	return ap_get_position(player);
}

/*
 * Class:     danbroid_libavplayer_LibAV
 * Method:    setDataSource
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_danbroid_libavplayer_LibAV__1setDataSource(
        JNIEnv *env, jclass cls, jlong handle, jstring jdatasource) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}

	const char *dataSource = (*env)->GetStringUTFChars(env, jdatasource, 0);
	assert(dataSource);

	ap_set_datasource(player, dataSource);

	(*env)->ReleaseStringUTFChars(env, jdatasource, dataSource);

}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_prepareAsync(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_prepare_async(player);
}

JNIEXPORT jboolean JNICALL Java_danbroid_libavplayer_LibAV_isLooping(
        JNIEnv *env, jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return JNI_FALSE;
	}

	return ap_is_looping(player);
}

JNIEXPORT void JNICALL Java_danbroid_libavplayer_LibAV_setLooping(JNIEnv *env,
        jclass cls, jlong handle, jboolean looping) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	ap_set_looping(player, looping);
}

/*
 * Class:     danbroid_libavplayer_LibAV
 * Method:    isPlaying
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_danbroid_libavplayer_LibAV_isPlaying(
        JNIEnv *env, jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return JNI_FALSE;
	}

	return ap_is_playing(player);
}

JNIEXPORT jint JNICALL Java_danbroid_libavplayer_LibAV_seekTo(JNIEnv *env,
        jclass cls, jlong handle, jint msecs) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	ap_seek(player, msecs, 0);
	return 0;
}

