package danbroid.libavplayer;

public class LibAndrudio {
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

  public static native int getDuration(long handle);

  public static native int getPosition(long handle);

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
