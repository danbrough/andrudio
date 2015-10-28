package danbroid.andrudio;

import java.util.Map;

/**
 * This is the low-level interface to the native code and comprises the entry
 * points of all native code used.
 * 
 */
public class LibAndrudio {

  /**
   * Callback interface for the native code. The native code calls these java
   * methods only.
   */
  public interface NativeCallbacks {

    public static final int EVENT_THREAD_START = 1;
    public static final int EVENT_STATE_CHANGE = 2;
    public static final int EVENT_SEEK_COMPLETE = 3;

    /**
     * Initialise the audio output
     * 
     * @param sampleFormat
     * @param sampleRateInHZ
     * @param channelConfig
     */
    void prepareAudio(int sampleFormat, int sampleRateInHZ, int channelConfig);

    void handleEvent(int what, int arg1, int arg2);

    /**
     * Play some PCM data
     * 
     * @param data
     * @param offset
     * @param length
     */
    void writePCM(byte data[], int offset, int length);
  }

  private static boolean initialized = false;

  public static void initialize() {

    // String libs[] = { "crypto", "ssl", "avutil", "avresample", "avcodec",
    // "avformat", "andrudio" };
    String libs[] = { "avutil", "avresample", "avcodec", "avformat", "andrudio" };

    for (int i = 0; i < libs.length; i++) {
      System.loadLibrary(libs[i]);
    }

    initializeLibrary(NativeCallbacks.class);
    initialized = true;
  }

  private static native int initializeLibrary(Class<NativeCallbacks> listenerClass);

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

  public static native void setListener(long handle, NativeCallbacks listener);

  public static native void destroy(long handle);

  public static native int prepareAsync(long handle) throws IllegalStateException;

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
  public static native int seekTo(long handle, int msecs, boolean relative);

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

  public static native int getMetaData(long handle, Map<String, String> data);

  // public static native int setUserAgent(long handle, String userAgent);

}
