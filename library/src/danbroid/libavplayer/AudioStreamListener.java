package danbroid.libavplayer;

public interface AudioStreamListener {

  public static final int EVENT_THREAD_START = 1;
  public static final int EVENT_STATE_CHANGE = 2;
  public static final int EVENT_SEEK_COMPLETE = 3;

  void writeAudio(byte data[], int offset, int len);

  void prepareAudio(int sampleFormat, int sampleRateInHZ, int channelConfig);

  void handleEvent(int what, int arg1, int arg2);
}
