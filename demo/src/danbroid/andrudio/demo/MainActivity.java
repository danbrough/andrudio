package danbroid.andrudio.demo;

import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLoggerFactory;

import android.annotation.TargetApi;
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
import danbroid.andrudio.AudioPlayer;
import danbroid.andrudio.LibAndrudio;

public class MainActivity extends AppCompatActivity {
  private static org.slf4j.Logger log = null;

  static {
    AndroidLoggerFactory.configureDefaultLogger(Package.getPackage("danbroid"));
    log = LoggerFactory.getLogger(MainActivity.class);
    LibAndrudio.initialize();

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

  private AudioPlayer player;
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

        log.debug("player.seekTo() {}", seekProgress);
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
          log.trace("progress: " + progress);
        }
      }
    });
    seekBar.setEnabled(false);
    seekBar.setProgress(0);

    // 48khz ogg
    addURL("http://h1.danbrough.org/media/tests/test48.ogg");

    // music that matters
    addURL("http://live-aacplus-64.kexp.org/kexp64.aac");

    addURL("http://stream.radioactive.fm:8000/ractive");
    // wmav2 encoded asf stream
    addURL("mmsh://streaming.radionz.co.nz/national-mbr");
    // rtsp aac stream
    addURL("rtsp://radionz-wowza.streamguys.com/national/national.stream");
    // mp3 stream
    addURL("http://radionz-ice.streamguys.com/national.mp3");
    // audio/aacp
    addURL("http://radionz-ice.streamguys.com/national");

    // local tests
    addURL("http://192.168.1.2/test.ogg");
    addURL("http://192.168.1.2/test.mp3");

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

  }

  private void addURL(final String url) {
    Button btn = new Button(this);
    btn.setText(url);
    buttons.addView(btn);
    btn.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        log.trace("playing: {}", url);
        player.play(url);
      }
    });
  }

  @Override
  protected void onStart() {
    log.info("onStart();");
    super.onStart();

    player = new AudioPlayer() {
      @Override
      public void onPeriodicNotification(android.media.AudioTrack track) {
        onUpdate();
      }

      @Override
      protected void onSeekComplete() {
        super.onSeekComplete();
        isSeeking = false;
      }

      @Override
      protected void onPrepared() {
        super.onPrepared();
        int duration = player.getDuration();
        seekBar.setProgress(0);
        if (duration > 0) {
          seekBar.setEnabled(true);
          seekBar.setMax(duration);
        } else {
          seekBar.setEnabled(false);
        }
        start();
      }
    };

  }

  private void onUpdate() {
    int duration = player.getDuration();
    int position = player.getPosition();

    player.printStatus();

    if (isSeeking) {
      log.trace("isSeeking .. wont update seekbar");
      return;
    }

    if (duration > 0) {
      seekBar.setProgress(position);
    }
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
