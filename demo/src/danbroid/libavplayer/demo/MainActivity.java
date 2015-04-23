package danbroid.libavplayer.demo;

import java.io.IOException;

import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLoggerFactory;

import android.annotation.TargetApi;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnSeekCompleteListener;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.SeekBar;
import danbroid.libavplayer.LibAVMediaPlayer;
import danbroid.libavplayer.LibAVMediaPlayer.OnStatusUpdateListener;

public class MainActivity extends AppCompatActivity {
  private static org.slf4j.Logger log = null;

  static {
    AndroidLoggerFactory.configureDefaultLogger(Package.getPackage("danbroid"));
    log = LoggerFactory.getLogger(MainActivity.class);
    danbroid.libavplayer.LibAVMediaPlayer.initJNI();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
      init_gingerbread();
    }
  }

  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  private static void init_gingerbread() {
    log.warn("setting strict thread policy");
    StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder()
        .detectAll().penaltyLog().build();
    StrictMode.setThreadPolicy(policy);
  }

  private MediaPlayer player;
  private ViewGroup buttons;
  private SeekBar seekBar;
  private boolean isSeeking = false;
  private int seekProgress = 0;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    buttons = (ViewGroup) findViewById(R.id.buttons);
    seekBar = (SeekBar) findViewById(R.id.seekBar);

    seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {
        log.trace("onStopTrackingTouch()");
        isSeeking = false;
        player.seekTo(seekProgress);
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {
        log.trace("onStartTrackingTouch()");
        isSeeking = true;
      }

      @Override
      public void onProgressChanged(SeekBar seekBar, int progress,
          boolean fromUser) {
        if (fromUser) {
          seekProgress = progress;
        }
      }
    });
    seekBar.setEnabled(false);
    seekBar.setProgress(0);

    addURL("http://192.168.1.2/test.mp3");
    addURL("http://h1.danbrough.org/media/tests/test48.ogg");
    addURL("http://live-aacplus-64.kexp.org/kexp64.aac");
    addURL("http://192.168.1.2/test.ogg");
    addURL("mmsh://streaming.radionz.co.nz/national-mbr");
    addURL("http://stream.radioactive.fm:8000/ractive");
    addURL("mmsh://streaming.radionz.co.nz/national-mbr");
    addURL("rtsp://radionz-wowza.streamguys.com/national/national.stream/");
    addURL("http://radionz-ice.streamguys.com/national.mp3");
    addURL("http://radionz-ice.streamguys.com/national");

    findViewById(R.id.pause).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.pause();
      }
    });
    findViewById(R.id.play).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.start();
      }
    });
    findViewById(R.id.stop).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.stop();
      }
    });
    findViewById(R.id.reset).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.reset();
      }
    });
  }

  private void addURL(final String url) {
    Button btn = new Button(this);
    btn.setText(url);
    buttons.addView(btn);
    btn.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        log.trace("playing: {}", url);
        player.reset();
        try {
          player.setDataSource(url);
        } catch (IOException e) {
          log.error(e.getMessage(), e);
          return;
        }
        player.prepareAsync();
      }
    });
  }

  @Override
  protected void onStart() {
    log.info("onStart();");
    super.onStart();
    player = new LibAVMediaPlayer();

    player.setOnPreparedListener(new OnPreparedListener() {
      @Override
      public void onPrepared(MediaPlayer mp) {
        player.start();
      }
    });
    player.setOnSeekCompleteListener(new OnSeekCompleteListener() {

      @Override
      public void onSeekComplete(MediaPlayer mp) {
        log.info("onSeekComplete();");
      }
    });

    ((LibAVMediaPlayer) player)
        .setOnStatusUpdateListener(new OnStatusUpdateListener() {
          @Override
          public void onStatusUpdate(int position, int duration) {

            if (duration > 0) {
              seekBar.setMax(duration);
              seekBar.setEnabled(true);
            } else {
              seekBar.setEnabled(false);
            }

            if (!isSeeking)
              seekBar.setProgress(position);
          }
        });
  }

  @Override
  protected void onStop() {
    log.info("onStop();");
    super.onStop();
    player.release();
    System.gc();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.main, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    // Handle action bar item clicks here. The action bar will
    // automatically handle clicks on the Home/Up button, so long
    // as you specify a parent activity in AndroidManifest.xml.
    int id = item.getItemId();
    if (id == R.id.action_settings) {
      return true;
    }
    return super.onOptionsItemSelected(item);
  }
}
