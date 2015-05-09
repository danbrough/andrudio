#include "audioplayer.h"
#include "danbroid_andrudio_LibAndrudio.h"
#include <assert.h>

#define JLONG_TO_PLAYER(handle) (player_t*)(intptr_t) handle
#define PLAYER_TO_JLONG(stream) (jlong)(intptr_t) stream

typedef struct fields_t {
	jclass class_audio_stream;
	jmethodID prepareAudio;
	jmethodID writeAudio;
	jmethodID onStateChanged;
	jmethodID handleEvent;
} fields_t;

typedef struct _JavaInfo {
	jobject listener;
	jobject audioTrack;
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

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_initializeLibrary(
        JNIEnv *env, jclass jCls, jclass listenerCls) {
	log_info("Java_danbroid_andrudio_LibAndrudio_initializeLibrary()");


	fields.class_audio_stream = listenerCls;

	fields.prepareAudio = (*env)->GetMethodID(env, listenerCls, "prepareAudio",
	        "(III)Landroid/media/AudioTrack;");

	fields.handleEvent = (*env)->GetMethodID(env, listenerCls, "handleEvent",
	        "(III)V");

	jclass audioTrackClass = (*env)->FindClass(env, "android/media/AudioTrack");
	if (audioTrackClass)
		fields.writeAudio = (*env)->GetMethodID(env, audioTrackClass,
		        "write", "([BII)I");

	return ap_init();
}

static void callback_on_play(player_t *player, char *data, int len) {
	///log_trace("callback_on_play() %d", len);

	JavaInfo *info = (JavaInfo*) player->extra;

	JNIEnv *env = get_jni_env();
	if (info->size < len) {
		log_debug("callback_on_play::old buffer too small");
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

	if (info->audioTrack) {
		(*env)->CallIntMethod(env, info->audioTrack, fields.writeAudio,
		        info->buffer, 0, len);
	}
}

static int callback_prepare_audio(player_t *player, int sampleFormat,
        int sampleRate, int channelFormat) {
	log_trace("callback_prepare_audio()");
	JNIEnv *env = get_jni_env();

	JavaInfo *info = (JavaInfo*) player->extra;

	if (info->audioTrack) {
		(*env)->DeleteGlobalRef(env, info->audioTrack);
	}

	info->audioTrack = (*env)->CallObjectMethod(env, info->listener,
	        fields.prepareAudio, sampleFormat, sampleRate, channelFormat);

	info->audioTrack = (*env)->NewGlobalRef(env, info->audioTrack);

	jclass audioTrackClass = (*env)->FindClass(env, "android/media/AudioTrack");

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

JNIEXPORT jlong JNICALL Java_danbroid_andrudio_LibAndrudio__1create(JNIEnv *env,
        jclass jCls) {
	log_info("Java_danbroid_andrudio_LibAndrudio__1create()");
	player_callbacks_t callbacks;
	memset(&callbacks, 0, sizeof(player_callbacks_t));

	callbacks.on_play = callback_on_play;
	callbacks.on_prepare = callback_prepare_audio;
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
JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio_destroy(JNIEnv *env,
        jclass jCls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	log_info("Java_danbroid_andrudio_LibAndrudio_destroy()");

	JavaInfo *info = (JavaInfo*) player->extra;

	ap_delete(player);

	jbyteArray buf = info->buffer;
	if (info) {
		if (info->buffer) {
			log_trace(
			        "Java_danbroid_andrudio_LibAndrudio_destroy::(*env)->DeleteGlobalRef(env, info->buffer);");
			(*env)->DeleteGlobalRef(env, info->buffer);
		}
		info->buffer = NULL;

		if (info->listener) {
			log_trace(
			        "Java_danbroid_andrudio_LibAndrudio_destroy::(*env)->DeleteGlobalRef(env, info->listener);");
			(*env)->DeleteGlobalRef(env, info->listener);
		}

		if (info->audioTrack) {
			log_trace(
			        "Java_danbroid_andrudio_LibAndrudio_destroy::(*env)->DeleteGlobalRef(env, info->audioTrack)");
			(*env)->DeleteGlobalRef(env, info->audioTrack);
		}
	}

	log_trace("Java_danbroid_andrudio_LibAndrudio_destroy::free(info)");
	free(info);
	log_trace("Java_danbroid_andrudio_LibAndrudio_destroy::done");

}
JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio_setListener(JNIEnv *env,
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

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_stop(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}
	return ap_stop(player);
}

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_reset(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	ap_reset(player);
	return 0;
}

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_start(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_start(player);
}

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_togglePause(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return (jint) ap_pause(player);
}

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_getDuration(JNIEnv *env,
        jclass cls, jlong handle) {
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_get_duration(player);
}

/*
 * Class:     danbroid_andrudio_LibAndrudio
 * Method:    getPosition
 * Signature: (J)J
 */
JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_getPosition(JNIEnv *env,
        jclass cls, jlong handle) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}
	return ap_get_position(player);
}

/*
 * Class:     danbroid_andrudio_LibAndrudio
 * Method:    setDataSource
 * Signature: (JLjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio__1setDataSource(
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

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_prepareAsync(JNIEnv *env,
        jclass cls, jlong handle) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	return ap_prepare_async(player);
}

JNIEXPORT jboolean JNICALL Java_danbroid_andrudio_LibAndrudio_isLooping(
        JNIEnv *env, jclass cls, jlong handle) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return JNI_FALSE;
	}

	return ap_is_looping(player);
}

JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio_setLooping(JNIEnv *env,
        jclass cls, jlong handle, jboolean looping) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	ap_set_looping(player, looping);
}

/*
 * Class:     danbroid_andrudio_LibAndrudio
 * Method:    isPlaying
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_danbroid_andrudio_LibAndrudio_isPlaying(
        JNIEnv *env, jclass cls, jlong handle) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return JNI_FALSE;
	}

	return ap_is_playing(player);
}


JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_seekTo(JNIEnv *env,
        jclass cls, jlong handle, jint msecs,jboolean relative) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return -1;
	}

	ap_seek(player, msecs*1000, relative);
	return 0;
}

JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio_audioPrepared(
        JNIEnv *env, jclass cls, jlong handle, jobject jaudioTrack,
        jint sampleFormat, jint sampleRate, jint channelFormat) {

	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}

	JavaInfo *info = (JavaInfo*) player->extra;
	info->audioTrack = (*env)->NewGlobalRef(env, jaudioTrack);
}

JNIEXPORT void JNICALL Java_danbroid_andrudio_LibAndrudio_printStatus
  (JNIEnv *env, jclass cls, jlong handle){
	player_t* player = JLONG_TO_PLAYER(handle);
	if (!player) {
		log_error("invalid handle");
		return;
	}
	ap_print_status(player);
}

JNIEXPORT jint JNICALL Java_danbroid_andrudio_LibAndrudio_getMetaData
  (JNIEnv *env, jclass cls, jobject map){
	//TODO
	log_error("Java_danbroid_andrudio_LibAndrudio_getMetaData() not implemented");
	return -1;
}
