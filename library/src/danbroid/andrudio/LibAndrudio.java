package danbroid.andrudio;

import android.media.AudioTrack;

public class LibAndrudio {

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

  private static boolean initialized = false;

  public static void initialize() {
    String libs[] = { "avutil", "avresample", "avcodec", "avformat", "andrudio" };

    for (String lib : libs) {
      System.loadLibrary(lib);
    }

    initializeLibrary(AudioStreamListener.class);
    initialized = true;
  }

  private static native int initializeLibrary(
      Class<AudioStreamListener> listenerClass);

  public static long create() {
    if (!initialized) {
      synchronized (LibAndrudio.class) {
        if (!initialized)
          initialize();
      }
    }
    return _create();
  }

  private static native long _create();

  public static native void setListener(long handle,
      AudioStreamListener listener);

  public static native void destroy(long handle);

  public static native int prepareAsync(long handle)
      throws IllegalStateException;

  public static native int start(long handle);

  public static native int stop(long handle);

  public static native int reset(long handle);

  public static native int togglePause(long handle);

  /**
   *
   * @return length of track in millis or -1 if track is a stream
   */
  public static native int getDuration(long handle);

  /**
   * 
   * @param handle
   * @return playback position in millis or -1 if the track is invalid
   */
  public static native int getPosition(long handle);

  /**
   * Seek to the specified position in millis
   * 
   * @param handle
   * @param msecs
   * @return 0 if successful
   */
  public static native int seekTo(long handle, int msecs);

  public static void setDataSource(long handle, String dataSource) {
    if (dataSource == null)
      throw new IllegalArgumentException("datasource is null");
    if (dataSource.startsWith("mms:"))
      dataSource = dataSource.replace("mms:", "mmsh:");
    _setDataSource(handle, dataSource);
  }

  private static native void _setDataSource(long handle, String dataSource);

  public static native boolean isLooping(long handle);

  public static native void setLooping(long handle, boolean looping);

  public static native boolean isPlaying(long handle);

}
