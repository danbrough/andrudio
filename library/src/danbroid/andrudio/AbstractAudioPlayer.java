package danbroid.andrudio;

import java.util.Map;

/**
 * 
 * This is the second tier API that resides on top of {@link LibAndrudio}.
 * You don't have to use this class, instead you can use {@link LibAndrudio}
 * directly to create your own API.
 */

public abstract class AbstractAudioPlayer implements
    LibAndrudio.NativeCallbacks {

  private long handle = 0;

  private State state;

  public enum State {
    IDLE, INITIALIZED, PREPARING, PREPARED, STARTED, PAUSED, COMPLETED, STOPPED, ERROR, END;
  }

  private static final State[] stateValues;
  static {
    stateValues = State.values();
  }

  public AbstractAudioPlayer() {
    super();
    handle = LibAndrudio.create();
    LibAndrudio.setListener(handle, this);
  }

  protected void onStateChange(State oldState, State state) {
    this.state = state;

    if (state == State.STOPPED || state == State.END) {
      onStopped();
    } else if (state == State.STARTED) {
      onStarted();
    } else if (state == State.PAUSED) {
      onPaused();
    } else if (state == State.PREPARED) {
      onPrepared();
    } else if (state == State.COMPLETED) {
      onCompleted();
    }
  }

  public State getState() {
    return state;
  }

  /**
   * resets, sets the datasource and prepares the player
   * 
   * @param url
   * the datasource
   */

  public void play(String url) {
    reset();
    setDataSource(url);
    prepareAsync();
  }

  public void prepareAsync() {
    LibAndrudio.prepareAsync(handle);
  }

  public synchronized void reset() {
    LibAndrudio.reset(handle);
  }

  public synchronized void release() {
    if (handle != 0) {
      LibAndrudio.destroy(handle);
      handle = 0;
    }
  }

  @Override
  protected void finalize() throws Throwable {
    if (handle != 0)
      release();
    super.finalize();
  }

  @Override
  public abstract void prepareAudio(int sampleFormat, int sampleRateInHZ,
      int channelConfig);

  @Override
  public final void handleEvent(int what, int arg1, int arg2) {
    switch (what) {
    case EVENT_SEEK_COMPLETE:
      onSeekComplete();
      break;
    case EVENT_STATE_CHANGE:
      onStateChange(stateValues[arg1], stateValues[arg2]);
      break;
    }
  }

  public void seekTo(int msecs) {
    LibAndrudio.seekTo(handle, msecs, false);
  }

  public void start() {
    LibAndrudio.start(handle);
  }

  public void stop() {
    LibAndrudio.stop(handle);
  }

  public void pause() {
    LibAndrudio.togglePause(handle);
  }

  protected abstract void onPrepared();

  protected abstract void onCompleted();

  protected abstract void onPaused();

  protected abstract void onStopped();

  protected abstract void onStarted();

  protected abstract void onSeekComplete();

  /**
   * 
   * @return playback position in millis or -1 if track is invalid
   */

  public int getPosition() {
    return LibAndrudio.getPosition(handle);
  }

  /**
   *
   * @return length of track in millis
   */
  public int getDuration() {
    return LibAndrudio.getDuration(handle);
  }

  public void setDataSource(String url) {
    LibAndrudio.setDataSource(handle, url);
  }

  public boolean isLooping() {
    return LibAndrudio.isLooping(handle);
  }

  public boolean isPaused() {
    return state == State.PAUSED;
  }

  public boolean isStarted() {
    return state == State.STARTED;
  }

  public void getMetaData(Map<String, String> map) {
    LibAndrudio.getMetaData(handle, map);
  }
}