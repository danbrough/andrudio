package danbroid.andrudio;

import java.io.FileDescriptor;
import java.io.IOException;
import java.util.Map;

import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.util.Log;
import danbroid.andrudio.AudioPlayer.State;

/**
 * This is the third tier API that sits on top of {@link MediaPlayer} and serves
 * as a drop in replacement for {@link MediaPlayer}.
 * TODO: Unfinished
 * 
 * @author dan
 *
 */
public class AndrudioMediaPlayer extends MediaPlayer {
  private static final String TAG = AndrudioMediaPlayer.class.getName();

  private final AudioPlayer player;
  private OnPreparedListener preparedListener;

  private OnSeekCompleteListener onSeekCompleteListener;

  public AndrudioMediaPlayer() {
    super();

    player = new AudioPlayer(new AudioPlayer.AudioPlayerListener() {
      @Override
      public void onStateChange(AudioPlayer player, State old, State state) {
        if (state == State.PREPARED && preparedListener != null)
          preparedListener.onPrepared(AndrudioMediaPlayer.this);
      }

      @Override
      public void onSeekComplete(AudioPlayer player) {
        if (onSeekCompleteListener != null)
          onSeekCompleteListener.onSeekComplete(AndrudioMediaPlayer.this);
      }

      @Override
      public void onPeriodicNotification(AudioPlayer player) {

      }
    });
  }

  @Override
  public void setOnPreparedListener(OnPreparedListener listener) {
    this.preparedListener = listener;
  }

  @Override
  public void setOnSeekCompleteListener(OnSeekCompleteListener listener) {
    this.onSeekCompleteListener = listener;
  }

  @Override
  public void setDataSource(Context context, Uri uri,
      Map<String, String> headers) throws IOException,
      IllegalArgumentException, SecurityException, IllegalStateException {
    Log.e(TAG, "setDataSource() Not implemented");
  }

  @Override
  public void setDataSource(FileDescriptor fd) throws IOException,
      IllegalArgumentException, IllegalStateException {
    Log.e(TAG, "setDataSource() Not implemented");
  }

  @Override
  public void setDataSource(String path) throws IOException,
      IllegalArgumentException, SecurityException, IllegalStateException {
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
    return player.isLooping();
  }

  @Override
  public int getCurrentPosition() {
    return player.getPosition();
  }

  @Override
  public int getDuration() {
    return player.getDuration();
  }

  @Override
  public void seekTo(int msec) throws IllegalStateException {
    player.seekTo(msec);
  }

  @Override
  public void pause() throws IllegalStateException {
    player.pause();
  }
}
