package danbroid.andrudio;

import java.io.FileDescriptor;
import java.io.IOException;
import java.util.Map;

import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.util.Log;

/**
 * This is a drop-in replacement for a {@link MediaPlayer} with limited
 * functionality
 *
 */
public class AndrudioMediaPlayer extends MediaPlayer {
  private static final String TAG = AndrudioMediaPlayer.class.getName();

  private final AndroidAudioPlayer player;
  private OnPreparedListener onPreparedListener;

  private OnSeekCompleteListener onSeekCompleteListener;

  public AndrudioMediaPlayer() {
    super();

    player = new AndroidAudioPlayer() {
      @Override
      protected void onSeekComplete() {
        if (onSeekCompleteListener != null)
          onSeekCompleteListener.onSeekComplete(AndrudioMediaPlayer.this);
      }

      @Override
      protected void onPrepared() {
        if (onPreparedListener != null)
          onPreparedListener.onPrepared(AndrudioMediaPlayer.this);
      }
    };
    player.setStatusUpdateInterval(0);
  }

  @Override
  public void setOnPreparedListener(OnPreparedListener listener) {
    this.onPreparedListener = listener;
  }

  @Override
  public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
    this.onSeekCompleteListener = listener;
  }

  @Override
  public void setDataSource(Context context, Uri uri, Map<String, String> headers)
      throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
    Log.e(TAG, "setDataSource() Not implemented");
  }

  @Override
  public void setDataSource(FileDescriptor fd) throws IOException, IllegalArgumentException, IllegalStateException {
    Log.e(TAG, "setDataSource() Not implemented");
  }

  @Override
  public void setDataSource(String path)
      throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
    player.setDataSource(path);
  }

  @Override
  public void prepare() throws IOException, IllegalStateException {
    Log.e(TAG, "prepare() not implemented");
  }

  @Override
  public void prepareAsync() throws IllegalStateException {
    player.prepareAsync();
  }

  @Override
  public void reset() {
    player.reset();
  }

  @Override
  public void release() {
    player.release();
  }

  @Override
  public void start() throws IllegalStateException {
    player.start();
  }

  @Override
  public void stop() throws IllegalStateException {
    player.stop();
  }

  @Override
  public boolean isLooping() {
    return player == null ? false : player.isLooping();
  }

  @Override
  public boolean isPlaying() {
    return player == null ? false : player.isStarted();
  }

  @Override
  public int getCurrentPosition() {
    return player == null ? 0 : player.getPosition();
  }

  @Override
  public int getDuration() {
    return player == null ? 0 : player.getDuration();
  }

  @Override
  public void seekTo(int msec) throws IllegalStateException {
    player.seekTo(msec);
  }

  @Override
  public void pause() throws IllegalStateException {
    player.pause();
  }

  public void getMetaData(Map<String, String> map) {
    player.getMetaData(map);
  }
}
