package danbroid.libavplayer;

public class LibAV {

  public static int initialiseLibrary() {
    return initialiseLibrary(AudioStreamListener.class);
  }

  private static native int initialiseLibrary(
      Class<AudioStreamListener> listenerClass);

  public static native long create();

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

  /* public static native void setDataSource(long handler, String dataSource);

   public static native int prepareAsync(long handler);

   public static native int reset(long handler);

   public static native long getDuration(long handle);

   public static native void seekTo(long handle, long ms);*/
}
