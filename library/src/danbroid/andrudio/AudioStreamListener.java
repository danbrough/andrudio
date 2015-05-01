package danbroid.andrudio;

import android.media.AudioTrack;

public interface AudioStreamListener {
  public static final int STATE_IDLE = 0;
  public static final int STATE_INITIALIZED = 1;
  public static final int STATE_PREPARING = 2;
  public static final int STATE_PREPARED = 3;
  public static final int STATE_STARTED = 4;
  public static final int STATE_PAUSED = 5;
  public static final int STATE_COMPLETED = 6;
  public static final int STATE_STOPPED = 7;
  public static final int STATE_ERROR = 8;
  public static final int STATE_END = 9;

  public static final int EVENT_THREAD_START = 1;
  public static final int EVENT_STATE_CHANGE = 2;
  public static final int EVENT_SEEK_COMPLETE = 3;

  AudioTrack prepareAudio(int sampleFormat, int sampleRateInHZ,
      int channelConfig);

  void handleEvent(int what, int arg1, int arg2);
}
