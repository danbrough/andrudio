package danbroid.andrudio.demo;

import android.annotation.TargetApi;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.preference.PreferenceManager;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import org.slf4j.LoggerFactory;
import org.slf4j.impl.AndroidLoggerFactory;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.HashMap;
import java.util.Locale;

import danbroid.andrudio.AndroidAudioPlayer;
import danbroid.andrudio.LibAndrudio;

public class MainActivity extends AppCompatActivity {
  private static org.slf4j.Logger log = null;

  static {
    AndroidLoggerFactory.configureDefaultLogger(Package.getPackage("danbroid.andrudio"));
    log = LoggerFactory.getLogger(MainActivity.class);
    LibAndrudio.initialize();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD && BuildConfig.DEBUG) {
      init_gingerbread();
    }
  }

  private final HashMap<String, String> metadata = new HashMap<String, String>();
  private AndroidAudioPlayer player;
  private ListView urls;
  private SeekBar seekBar;
  private boolean isSeeking = false;
  private int seekProgress = 0;
  private ArrayAdapter<MenuAction> adapter;
  private SharedPreferences prefs;

  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  private static void init_gingerbread() {
    log.warn("setting strict thread policy");
    StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder().detectAll().penaltyLog()
        .penaltyDialog().build());
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    prefs = PreferenceManager.getDefaultSharedPreferences(this);

    setContentView(R.layout.activity_main);

    urls = (ListView) findViewById(R.id.urls);
    this.adapter = new ArrayAdapter<MenuAction>(this, android.R.layout.simple_list_item_1) {
      @Override
      public View getView(int position, View convertView, ViewGroup parent) {
        View view = super.getView(position, convertView, parent);

        return view;
      }

    };
    adapter.setNotifyOnChange(false);
    urls.setAdapter(adapter);
    urls.setOnItemClickListener(new AdapterView.OnItemClickListener() {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        adapter.getItem(position).onClick();
      }
    });

    seekBar = (SeekBar) findViewById(R.id.seekBar);

    seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

      @Override
      public void onStopTrackingTouch(SeekBar seekBar) {
        log.debug("onStopTrackingTouch(): player.seekTo() {}", seekProgress);
        player.seekTo(seekProgress);
      }

      @Override
      public void onStartTrackingTouch(SeekBar seekBar) {
        log.trace("onStartTrackingTouch()");
        isSeeking = true;
      }

      @Override
      public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        if (fromUser) {
          seekProgress = progress;
          log.trace("progress: " + progress);
        }
      }
    });

    seekBar.setEnabled(false);
    seekBar.setProgress(0);

    findViewById(R.id.pause).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.pause();
      }
    });

    findViewById(R.id.stop).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        player.stop();
      }
    });

    /*
     * findViewById(R.id.start).setOnClickListener(new View.OnClickListener() {
     * 
     * @Override public void onClick(View v) { player.start(); } });
     * 
     * findViewById(R.id.reset).setOnClickListener(new View.OnClickListener() {
     * 
     * @Override public void onClick(View v) { player.reset(); } });
     */

    new AsyncTask<Void, Void, Void>() {

      @Override
      protected Void doInBackground(Void... params) {
        addCustomURLS();
        addHardCodedURLS();
        return null;
      }

      protected void onPostExecute(Void result) {
        adapter.notifyDataSetChanged();
      }

    }.execute();
  }

  private void addURL(final String url) {
    adapter.add(new MenuAction(url, "") {
      @Override
      public void onClick() {
        play(title, description, url);
      }
    });
  }

  public void play(final String title, final String description, final String url) {
    log.debug("play() {}:{}:" + url, title, description);

    int i = url.lastIndexOf('.');
    if (i > 0) {
      String extn = url.substring(i + 1).toLowerCase(Locale.US);
      if (extn.equals("m3u") || extn.equals("asf") || extn.equals("pls")) {
        new AsyncTask<Void, Void, Void>() {
          String newUrl = null;

          @Override
          protected Void doInBackground(Void... params) {
            try {
              newUrl = parsePlayList(url);
            } catch (IOException e) {
              log.error(e.getMessage(), e);
              Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_SHORT).show();
            }
            return null;
          }

          protected void onPostExecute(Void result) {
            if (newUrl != null)
              play(title, description, newUrl);
          }

        }.execute();
        return;
      }
    }

    Toast.makeText(this, description.equals("") ? url : description, Toast.LENGTH_SHORT).show();
    player.reset();
    player.setDataSource(url);
    player.prepareAsync();

  }

  private String parsePlayList(final String url) throws IOException {
    log.warn("parsePlayList(): {}", url);

    HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
    conn.connect();
    int code = conn.getResponseCode();
    if (code != HttpURLConnection.HTTP_OK) {
      throw new IOException("Http response code: " + code);
    }
    BufferedReader input = new BufferedReader(new InputStreamReader(conn.getInputStream()));
    String s = null;
    while ((s = input.readLine()) != null) {
      if (s.startsWith("File")) {
        int i = s.indexOf("=");
        if (i > 0) {
          return s.substring(i + 1).trim();
        }
      } else if (s.startsWith("http")) {
        return s.trim();
      }
    }

    throw new IOException("Failed to parse " + url);
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
        log.trace("onSeekComplete()");
        isSeeking = false;
      }

      @Override
      protected void onPrepared() {
        super.onPrepared();

        runOnUiThread(new Runnable() {
          @Override
          public void run() {
            log.trace("onPrepared::configuring seekBar");
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
      public synchronized void reset() {
        log.debug("reset();");
        super.reset();
        seekBar.setEnabled(false);
        seekBar.setProgress(0);
      }

      @Override
      public synchronized void stop() {
        log.debug("stop();");
        super.stop();
        seekBar.setEnabled(false);
        seekBar.setProgress(0);
      }

      @Override
      protected void onStatusUpdate() {
        MainActivity.this.onStatusUpdate();
      }
    };

  }

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

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    menu.add(0, R.id.menu_add_custom_url, 0, "Add Custom URL");
    return super.onCreateOptionsMenu(menu);
  }

  public void addCustomURL() {
    log.debug("addCustomURL();");
    CustomURLDialog dlg = new CustomURLDialog();
    dlg.show(getSupportFragmentManager(), "dialog");
  }

  public void addCustomURL(final String url) {
    log.info("addCustomURL() :{}", url);

    new AsyncTask<Void, Void, Void>() {
      @Override
      protected Void doInBackground(Void... params) {
        Editor editor = prefs.edit();
        int urlCount = prefs.getInt("urlCount", 0);
        editor.putString("CUSTOM_URL_" + urlCount, url);
        editor.putInt("urlCount", urlCount + 1);
        editor.commit();
        return null;
      }

      protected void onPostExecute(Void result) {
        addURL(url);
        adapter.notifyDataSetChanged();
      }
    }.execute();
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.menu_add_custom_url:
        addCustomURL();
        return true;
    }
    return super.onOptionsItemSelected(item);
  }

  private void addCustomURLS() {
    int index = 0;
    while (true) {
      String url = prefs.getString("CUSTOM_URL_" + index, "");
      if (url.equals(""))
        return;
      index++;
      addURL(url);
    }
  }

  private void addHardCodedURLS() {

    // andHow.fm
    addURL("http://www.andhow.fm/playlists/andhow.m3u");

    // WRUV 90.1 fm Burlington VT
    addURL("http://icecast.uvm.edu:8005/wruv_fm_256");

    // Radio One, 91FM in Dunedin
    addURL("http://stream.r1.co.nz:8090/r1-high.mp3");

    // 48khz ogg (a test with an odd sample rate)
    // this tune is driving me insane.
    addURL("http://h1.danbrough.org/media/tests/test48.ogg");

    // music that matters
    addURL("http://live-aacplus-64.kexp.org/kexp64.aac");

    // wellington student radio
    addURL("http://stream.radioactive.fm:8000/ractive");

    addURL("http://xstream1.somafm.com:8900");

    // audio/x-scpls
    addURL("http://somafm.com/deepspaceone.pls");

    // audio/x-mpegurl
    addURL("http://www.listenlive.eu/bbcradio1.m3u");

    // rtsp aac stream
    addURL("rtsp://radionz-wowza.streamguys.com/national/national.stream");

    // mp3 stream
    addURL("http://radionz-ice.streamguys.com/national.mp3");
    // audio/aacp
    addURL("http://radionz-ice.streamguys.com/national");
  }

  static class MenuAction {
    String title;
    String description;

    public MenuAction(String title, String description) {
      this.title = title;
      this.description = description;
    }

    public void onClick() {
      log.debug("clicked: {}", title);
    }

    @Override
    public String toString() {
      return title;
    }
  }

  public static class CustomURLDialog extends DialogFragment {
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
      AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
      builder.setTitle("Add Custom URL");
      builder.setView(R.layout.custom_url_dialog);

      builder.setPositiveButton(getString(android.R.string.ok), new DialogInterface
          .OnClickListener() {
        @Override
        public void onClick(DialogInterface d, int which) {
          CharSequence url = ((TextView) getDialog().findViewById(R.id.url)).getText();
          ((MainActivity) getActivity()).addCustomURL(url.toString());
        }
      });
      return builder.create();
    }
  }
}
