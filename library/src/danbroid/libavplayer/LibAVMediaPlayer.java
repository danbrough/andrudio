package danbroid.libavplayer;

import java.io.FileDescriptor;
import java.io.IOException;
import java.util.Map;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class LibAVMediaPlayer extends MediaPlayer {
  private static final String TAG = "danbroid.LibAVMediaPlayer";

  public enum AVSampleFormat {
    AV_SAMPLE_FMT_U8,          // /< unsigned 8 bits
    AV_SAMPLE_FMT_S16,         // /< signed 16 bits
    AV_SAMPLE_FMT_S32,         // /< signed 32 bits
    AV_SAMPLE_FMT_FLT,         // /< float
    AV_SAMPLE_FMT_DBL,         // /< double
    AV_SAMPLE_FMT_U8P,         // /< unsigned 8 bits, planar
    AV_SAMPLE_FMT_S16P,        // /< signed 16 bits, planar
    AV_SAMPLE_FMT_S32P,        // /< signed 32 bits, planar
    AV_SAMPLE_FMT_FLTP,        // /< float, planar
    AV_SAMPLE_FMT_DBLP,        // /< double, planar
  };

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

  public interface OnStatusUpdateListener {
    void onStatusUpdate(int position, int duration);
  }

  public static void initJNI() {
    String libs[] = { "avutil", "avresample", "avcodec", "avformat", "avplayer" };

    for (String lib : libs) {
      System.loadLibrary(lib);
    }

    LibAV.initialiseLibrary();
  }

  private static final int EVENT_STATUS = 1000;

  private final Handler handler = new Handler(new Handler.Callback() {
    @Override
    public boolean handleMessage(Message msg) {
      switch (msg.what) {
      case AudioStreamListener.EVENT_STATE_CHANGE:
        onStateChange(msg.arg1, msg.arg2);
        break;
      case AudioStreamListener.EVENT_SEEK_COMPLETE:
        if (onSeekCompleteListener != null)
          onSeekCompleteListener.onSeekComplete(LibAVMediaPlayer.this);
        break;
      case EVENT_STATUS:
        if (statusPollLength > 0)
          onStatusUpdate(getCurrentPosition(), getDuration());
        break;
      }
      return false;
    }
  });

  private long handle = 0L;
  private AudioTrack audioTrack;
  private OnSeekCompleteListener onSeekCompleteListener;
  private OnPreparedListener onPreparedListener;
  private OnStatusUpdateListener onStatusUpdateListener;
  private long statusPollLength = 1000;

  private boolean playing = false;

  public LibAVMediaPlayer() {
    super();
    handle = LibAV.create();
    LibAV.setListener(handle, new AudioStreamListener() {

      int prevSampleFormat = 0;
      int prevSampleRate = 0;
      int prevChannelConfig = 0;

      static final String TAG = "danbroid.AudioStreamListener";

      @Override
      public void writeAudio(byte[] data, int offset, int len) {
        audioTrack.write(data, offset, len);
      }

      @Override
      public void onPrepared(final int sampleFormat, final int sampleRateInHz,
          final int channelConfig) {

        handler.post(new Runnable() {
          @Override
          public void run() {
            Log.i(TAG, "onPrepared():" + Thread.currentThread().getId()
                + " sampleFormat: " + sampleFormat + " sampleRateInHZ: "
                + sampleRateInHz + " channelConfig: " + channelConfig);
            boolean configChanged = false;

            int chanConfig = (channelConfig == 1) ? AudioFormat.CHANNEL_OUT_MONO
                : AudioFormat.CHANNEL_OUT_STEREO;

            configChanged = sampleFormat != prevSampleFormat
                | chanConfig != prevChannelConfig
                | sampleRateInHz != prevSampleRate;
            prevChannelConfig = chanConfig;
            prevSampleFormat = sampleFormat;
            prevSampleRate = sampleRateInHz;

            int minBufferSize = AudioTrack.getMinBufferSize(sampleRateInHz,
                chanConfig, AudioFormat.ENCODING_PCM_16BIT) * 4;
            Log.v(TAG, "minBufferSize: " + minBufferSize);

            if (configChanged && audioTrack != null) {
              audioTrack.release();
              audioTrack = null;
            }

            if (audioTrack == null) {
              audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                  sampleRateInHz, chanConfig, AudioFormat.ENCODING_PCM_16BIT,
                  minBufferSize, AudioTrack.MODE_STREAM);
            }
          }
        });
      }

      @Override
      public void handleEvent(int what, int arg1, int arg2) {
        handler.sendMessage(handler.obtainMessage(what, arg1, arg2));
      }
    });
  }

  protected void onStateChange(int old_state, int state) {
    if (state == STATE_STARTED) {
      playing = true;
      onStatusUpdate(getCurrentPosition(), getDuration());
    } else if (old_state == STATE_STARTED) {
      playing = false;
      handler.removeMessages(EVENT_STATUS);
      if (state != STATE_IDLE) {
        Log.v(TAG, "calling onStatusUpdate from state: " + state);
        onStatusUpdate(getCurrentPosition(), getDuration());
      }
    }

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
      if (onPreparedListener != null)
        onPreparedListener.onPrepared(LibAVMediaPlayer.this);
    } else if (state == STATE_COMPLETED) {
      Log.i(TAG, "onStateChange::STATE_COMPLETED");
    }
  }

  private void onStatusUpdate(int position, int duration) {
    Log.v(TAG, "onStatusUpdate() position: " + position + " duration:"
        + duration);
    if (onStatusUpdateListener != null)
      onStatusUpdateListener.onStatusUpdate(duration > 0 ? position : 0,
          duration);
    if (playing && duration > 0 && statusPollLength > 0)
      handler.sendEmptyMessageDelayed(EVENT_STATUS, statusPollLength);
  }

  public long getStatusPollLength() {
    return statusPollLength;
  }

  public void setStatusPollLength(long statusPollLength) {
    this.statusPollLength = statusPollLength;
  }

  @Override
  public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
    this.onPreparedListener = onPreparedListener;
  }

  @Override
  public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
    this.onSeekCompleteListener = listener;
  }

  public void setOnStatusUpdateListener(
      OnStatusUpdateListener onStatusUpdateListener) {
    this.onStatusUpdateListener = onStatusUpdateListener;
  }

  @Override
  public void setDataSource(String path) throws IOException,
      IllegalArgumentException, SecurityException, IllegalStateException {
    LibAV.setDataSource(handle, path);
  }

  @Override
  public void setDataSource(Context context, Uri uri) throws IOException,
      IllegalArgumentException, SecurityException, IllegalStateException {
    throw new IllegalArgumentException("not implemented");
  }

  @Override
  public void setDataSource(Context context, Uri uri,
      Map<String, String> headers) throws IOException,
      IllegalArgumentException, SecurityException, IllegalStateException {
    throw new IllegalArgumentException("not implemented");
  }

  @Override
  public void setDataSource(FileDescriptor fd) throws IOException,
      IllegalArgumentException, IllegalStateException {
    throw new IllegalArgumentException("not implemented");
  }

  @Override
  public void setDataSource(FileDescriptor fd, long offset, long length)
      throws IOException, IllegalArgumentException, IllegalStateException {
    throw new IllegalArgumentException("not implemented");
  }

  @Override
  public void prepare() throws IOException, IllegalStateException {
    throw new IllegalArgumentException("not implemented");
  }

  @Override
  public void prepareAsync() throws IllegalStateException {
    LibAV.prepareAsync(handle);
  }

  @Override
  public void pause() {
    Log.v(TAG, "pause()");
    LibAV.togglePause(handle);
  }

  @Override
  public void stop() {
    Log.v(TAG, "stop()");
    LibAV.stop(handle);
  }

  @Override
  public void start() {
    Log.v(TAG, "start()");
    LibAV.start(handle);
  }

  @Override
  public void reset() {
    Log.v(TAG, "reset()");
    LibAV.reset(handle);
  }

  @Override
  public boolean isLooping() {
    return LibAV.isLooping(handle);
  }

  @Override
  public void setLooping(boolean looping) {
    LibAV.setLooping(handle, looping);
  }

  @Override
  public void seekTo(int msec) throws IllegalStateException {
    LibAV.seekTo(handle, msec);
  }

  @Override
  public synchronized void release() {
    Log.i(TAG, "release()");
    if (handle != 0) {
      LibAV.destroy(handle);
    }
    handle = 0;
  }

  @Override
  protected void finalize() {
    if (handle != 0) {
      release();
    }
    super.finalize();
  }

  @Override
  public boolean isPlaying() {
    return LibAV.isPlaying(handle);
  }

  @Override
  public int getCurrentPosition() {
    return LibAV.getPosition(handle);
  }

  @Override
  public int getDuration() {
    return LibAV.getDuration(handle);
  }

}
