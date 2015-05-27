package danbroid.andrudio.demo;

import java.util.HashMap;

import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLoggerFactory;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.SeekBar;
import danbroid.andrudio.AndroidAudioPlayer;
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
    StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder()
        .detectAll().penaltyLog().penaltyDialog().build());
  }

  private AndroidAudioPlayer player;
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
        isSeeking = false;
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

    // 48khz ogg (a test with an odd sample rate)
    // this tune is driving me insane.
    addURL("http://h1.danbrough.org/media/tests/test48.ogg");
    // local tests
    addURL("http://192.168.1.2/test.ogg");
    addURL("http://192.168.1.2/test.mp3");

    // wmav2 encoded asf stream
    addURL("mmsh://streaming.radionz.co.nz/national-mbr");

    // music that matters
    addURL("http://live-aacplus-64.kexp.org/kexp64.aac");

    // wellington student radio
    addURL("http://stream.radioactive.fm:8000/ractive");

    // streams below aren't working too well or not at all

    // audio/x-mpegurl
    addURL("http://www.listenlive.eu/bbcradio1.m3u");

    // this is the url from the contents of
    // http://www.listenlive.eu/bbcradio1.m3u (audio/mpeg)
    addURL("http://bbcmedia.ic.llnwd.net/stream/bbcmedia_radio1_mf_p");

    // rtsp aac stream
    addURL("rtsp://radionz-wowza.streamguys.com/national/national.stream");

    // mp3 stream
    addURL("http://radionz-ice.streamguys.com/national.mp3");
    // audio/aacp
    addURL("http://radionz-ice.streamguys.com/national");

    findViewById(R.id.pause).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.pause();
      }
    });

    findViewById(R.id.start).setOnClickListener(new View.OnClickListener() {
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
        player.play(url);
      }
    });
  }

  @Override
  protected void onStart() {
    log.info("onStart();");
    super.onStart();
    if (player != null)
      player.release();

    player = new AndroidAudioPlayer() {
      @Override
      protected void onSeekComplete() {
      }

      @Override
      protected void onPrepared() {
        super.onPrepared();

        runOnUiThread(new Runnable() {
          @Override
          public void run() {
            int duration = getDuration();
            if (duration <= 0) {
              seekBar.setEnabled(false);
              seekBar.setProgress(0);
            } else {
              seekBar.setMax(duration);
              seekBar.setProgress(getPosition());
              seekBar.setEnabled(true);
            }
          }
        });
      }

      @Override
      protected void onStatusUpdate() {
        MainActivity.this.onStatusUpdate();
      }
    };

  }

  private final HashMap<String, String> metadata = new HashMap<String, String>();

  private void onStatusUpdate() {
    final int duration = player.getDuration();
    final int position = player.getPosition();
    log.trace("onStatusUpdate():" + position + ":" + duration);

    if (isSeeking) {
      log.trace("isSeeking .. wont update seekbar");
      return;
    }

    if (duration > 0) {
      seekBar.setProgress(position);
    }

    metadata.clear();

    player.getMetaData(metadata);
    for (String key : metadata.keySet()) {
      log.trace("metadata: {}:\t{}", key, metadata.get(key));
    }

  }

  @Override
  protected void onStop() {
    log.info("onStop();");
    super.onStop();
    player.release();
    player = null;
    System.gc();
  }

}
