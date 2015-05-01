package danbroid.andrudio;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioTrack.OnPlaybackPositionUpdateListener;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

/**
 * A simple audio player that wraps a {@link AudioTrack} instance
 * 
 * @author dan
 */
public class AudioPlayer implements AudioStreamListener,
    OnPlaybackPositionUpdateListener {

  private static final String TAG = AudioPlayer.class.getName();

  private long handle = 0;

  private int sampleFormat;

  private int sampleRateInHz;

  private int channelConfig;

  private AudioTrack audioTrack;

  private final Handler handler = new Handler(new Handler.Callback() {
    @Override
    public boolean handleMessage(Message msg) {
      switch (msg.what) {
      case EVENT_SEEK_COMPLETE:
        Log.i(TAG, "event seek complete");
        return true;
      case EVENT_STATE_CHANGE:
        onStateChange(msg.arg1, msg.arg2);
        return true;
      default:
        Log.e(TAG, "unhandled event: " + msg.what);
      }
      return false;
    }
  });

  public AudioPlayer() {
    super();
    handle = LibAndrudio.create();
    LibAndrudio.setListener(handle, this);
  }

  protected void onStateChange(int old_state, int state) {
    Log.v(TAG, "onStateChange() " + old_state + " -> " + state);

    if (old_state == STATE_STARTED && state != STATE_PAUSED
        && state != STATE_COMPLETED) {
      Log.v(TAG, "audioTrack.stop()");
      audioTrack.stop();
    } else if (state == STATE_STARTED) {
      Log.v(TAG, "audioTrack.play()");
      audioTrack.play();
    } else if (state == STATE_PAUSED) {
      Log.v(TAG, "audioTrack.pause()");
      audioTrack.pause();
    } else if (state == STATE_PREPARED) {
      onPrepared();
    } else if (state == STATE_COMPLETED) {
      Log.i(TAG, "onStateChange::STATE_COMPLETED calling stop");
      if (audioTrack != null)
        audioTrack.stop();
    }
  }

  public void play(String url) {
    Log.i(TAG, "play() :" + url);
    reset();
    LibAndrudio.setDataSource(handle, url);
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
    Log.i(TAG, "release()");
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
      audioTrack.setPlaybackPositionUpdateListener(this, handler);
      audioTrack.setPositionNotificationPeriod(sampleRateInHZ);
    }

    return audioTrack;
  }

  @Override
  public final void handleEvent(int what, int arg1, int arg2) {
    handler.sendMessage(handler.obtainMessage(what, arg1, arg2));
  }

  public AudioTrack getAudioTrack() {
    return audioTrack;
  }

  public void seekTo(int msecs) {
    LibAndrudio.seekTo(handle, msecs);
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

  protected void onPrepared() {
    Log.v(TAG, "onPrepared()");
    start();
  }

  @Override
  public void onMarkerReached(AudioTrack track) {
  }

  @Override
  public void onPeriodicNotification(AudioTrack track) {
    Log.v(TAG, "onPeriodicNotification() position:" + getPosition()
        + " head position: " + track.getPlaybackHeadPosition()
        + " playback rate: " + track.getPlaybackRate() + " duration: "
        + getDuration());
  }

  public int getPosition() {
    if (audioTrack == null)
      return -1;
    return audioTrack.getPlaybackHeadPosition() / audioTrack.getSampleRate();
  }

  public long getDuration() {
    return LibAndrudio.getDuration(handle);
  }
}