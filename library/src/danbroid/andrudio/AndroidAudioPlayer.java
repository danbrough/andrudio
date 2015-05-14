package danbroid.andrudio;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.AudioTrack.OnPlaybackPositionUpdateListener;
import android.util.Log;

/**
 * {@link AbstractAudioPlayer} subclass that uses a {@link AudioTrack} instance.
 *
 */

public class AndroidAudioPlayer extends AbstractAudioPlayer {

  private static final String TAG = AndroidAudioPlayer.class.getName();

  private int sampleFormat;

  private int sampleRateInHz;

  private int channelConfig;

  private AudioTrack audioTrack;

  private long statusUpdateInterval = 1000;

  @Override
  public synchronized void reset() {
    super.reset();
    if (audioTrack != null) {
      audioTrack.release();
      audioTrack = null;
    }
  }

  @Override
  public synchronized void prepareAudio(int sampleFormat, int sampleRateInHZ,
      int channelConfig) {
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

      onNewAudioTrack();
    }
  }

  protected void onNewAudioTrack() {
    if (statusUpdateInterval != 0) {
      int periodInFrames = (int) ((sampleRateInHz * statusUpdateInterval) / 1000);
      audioTrack.setPositionNotificationPeriod(periodInFrames);
      audioTrack
          .setPlaybackPositionUpdateListener(new OnPlaybackPositionUpdateListener() {

            @Override
            public void onPeriodicNotification(AudioTrack track) {
              onStatusUpdate();
            }

            @Override
            public void onMarkerReached(AudioTrack track) {
            }
          });
    } else {
      audioTrack.setPlaybackPositionUpdateListener(null);
    }
  }

  public AudioTrack getAudioTrack() {
    return audioTrack;
  }

  /**
   * Override to update playback status information.
   * 
   * @see AndroidAudioPlayer#statusUpdateInterval
   */
  protected void onStatusUpdate() {
    Log.i(TAG, "onStatusUpdate() " + getPosition() + ":" + getDuration());
  }

  /**
   * Set this to 0 to disable automatic status updates
   * 
   * @param statusUpdateInterval
   * millis between calls to {@link AndroidAudioPlayer#onStatusUpdate()}
   */
  public void setStatusUpdateInterval(long statusUpdateInterval) {
    this.statusUpdateInterval = statusUpdateInterval;
  }

  public long getStatusUpdateInterval() {
    return statusUpdateInterval;
  }

  @Override
  protected void onCompleted() {
    if (audioTrack != null) {
      Log.v(TAG, "audioTrack.stop()");
      audioTrack.stop();
    }
  }

  @Override
  protected void onPaused() {
    if (audioTrack != null) {
      Log.v(TAG, "audioTrack.pause()");
      audioTrack.pause();
    }
  }

  @Override
  protected void onStarted() {
    if (audioTrack != null) {
      Log.v(TAG, "audioTrack.play()");
      audioTrack.play();
    }
  }

  @Override
  public void writePCM(byte[] data, int offset, int length) {
    audioTrack.write(data, offset, length);
  }
}