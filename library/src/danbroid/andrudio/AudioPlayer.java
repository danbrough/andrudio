package danbroid.andrudio;

import java.util.Map;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioTrack.OnPlaybackPositionUpdateListener;
import android.util.Log;

/**
 * A simple audio player that wraps a {@link AudioTrack} instance.
 * This is the second tier API that resides on top of {@link LibAndrudio}.
 * 
 * 
 * @author dan
 */
public class AudioPlayer implements LibAndrudio.AudioStreamListener,
    OnPlaybackPositionUpdateListener {

  private static final String TAG = AudioPlayer.class.getName();

  private long handle = 0;

  private int sampleFormat;

  private int sampleRateInHz;

  private int channelConfig;

  private AudioTrack audioTrack;

  private State state;

  public enum State {
    IDLE, INITIALIZED, PREPARING, PREPARED, STARTED, PAUSED, COMPLETED, STOPPED, ERROR, END;
  }

  public interface AudioPlayerListener {
    void onStateChange(AudioPlayer player, State old, State state);

    void onSeekComplete(AudioPlayer player);

    void onPeriodicNotification(AudioPlayer player);
  }

  private AudioPlayerListener listener;

  private static final State[] stateValues;
  static {
    stateValues = State.values();
  }

  public AudioPlayer() {
    this(null);
  }

  public AudioPlayer(AudioPlayerListener listener) {
    super();
    handle = LibAndrudio.create();
    LibAndrudio.setListener(handle, this);
    if (listener != null)
      setListener(listener);
  }

  protected void onStateChange(State oldState, State state) {
    Log.v(TAG, "onStateChange() " + oldState + " -> " + state);
    this.state = state;

    if (listener != null)
      listener.onStateChange(this, oldState, state);

    if (oldState == State.STARTED && state != State.PAUSED) {
      Log.v(TAG, "audioTrack.stop()");
      audioTrack.stop();
    } else if (state == State.STARTED) {
      Log.v(TAG, "audioTrack.play()");
      audioTrack.play();
    } else if (state == State.PAUSED) {
      Log.v(TAG, "audioTrack.pause()");
      audioTrack.pause();
    } else if (state == State.PREPARED) {
      onPrepared();
    } else if (state == State.COMPLETED) {
      if (audioTrack != null) {
        Log.v(TAG, "audioTrack.stop()");
        audioTrack.stop();
      }
    }
  }

  public void setListener(AudioPlayerListener listener) {
    this.listener = listener;
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
    Log.i(TAG, "play() :" + url);
    reset();
    setDataSource(url);
    prepareAsync();
  }

  public void prepareAsync() {
    LibAndrudio.prepareAsync(handle);
  }

  public synchronized void reset() {
    LibAndrudio.reset(handle);
    if (audioTrack != null) {
      audioTrack.release();
      audioTrack = null;
    }
  }

  public synchronized void release() {
    Log.d(TAG, "release()");
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
  public synchronized AudioTrack prepareAudio(int sampleFormat,
      int sampleRateInHZ, int channelConfig) {
    Log.d(TAG, "prepareAudio() format: " + sampleFormat + " rate: "
        + sampleRateInHZ + " channels: " + channelConfig);

    boolean changed = (this.sampleFormat != sampleFormat
        || this.sampleRateInHz != sampleRateInHZ || this.channelConfig != channelConfig);

    this.sampleFormat = sampleFormat;
    this.channelConfig = channelConfig;
    this.sampleRateInHz = sampleRateInHZ;
    if (changed && this.audioTrack != null) {
      audioTrack.release();
      audioTrack = null;
    }
    int chanConfig = (channelConfig == 1) ? AudioFormat.CHANNEL_OUT_MONO
        : AudioFormat.CHANNEL_OUT_STEREO;

    int minBufferSize = AudioTrack.getMinBufferSize(sampleRateInHz, chanConfig,
        AudioFormat.ENCODING_PCM_16BIT) * 4;
    Log.v(TAG, "minBufferSize: " + minBufferSize);

    if (audioTrack == null) {
      audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz,
          chanConfig, AudioFormat.ENCODING_PCM_16BIT, minBufferSize,
          AudioTrack.MODE_STREAM);
      audioTrack.setPlaybackPositionUpdateListener(this);
      audioTrack.setPositionNotificationPeriod(sampleRateInHZ);
    }

    return audioTrack;
  }

  @Override
  public final void handleEvent(int what, int arg1, int arg2) {
    switch (what) {
    case EVENT_SEEK_COMPLETE:
      onSeekComplete();
      break;
    case EVENT_STATE_CHANGE:
      onStateChange(stateValues[arg1], stateValues[arg2]);
      break;
    default:
      Log.e(TAG, "handleEvent() " + what + ":" + arg1 + ":" + arg2
          + " not handled");
      break;
    }
  }

  public AudioTrack getAudioTrack() {
    return audioTrack;
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

  /**
   * Will call {@link #start()}. Do not call super to override this behaviour.
   */
  protected void onPrepared() {
    Log.v(TAG, "onPrepared()");
    onPeriodicNotification(audioTrack);
  }

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

  public void printStatus() {
    LibAndrudio.printStatus(handle);
  }

  protected void onSeekComplete() {
    Log.v(TAG, "onSeekComplete()");
    if (listener != null)
      listener.onSeekComplete(this);
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

  @Override
  public void onMarkerReached(AudioTrack track) {
  }

  @Override
  public void onPeriodicNotification(AudioTrack track) {
    if (listener != null)
      listener.onPeriodicNotification(this);
  }
}