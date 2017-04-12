package danbroid.andrudio.demo;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.Nullable;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Toast;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.util.LinkedList;
import java.util.zip.GZIPInputStream;

import danbroid.andrudio.AndrudioMediaPlayer;


public class MainActivity extends AppCompatActivity implements MediaPlayer.OnPreparedListener {
  private static final Logger log = LoggerFactory.getLogger(MainActivity.class);
  private static final int MENU_ADD_URL = 1000;
  private static final int MENU_RECENT = 1001;
  private static final String PREF_LAST_URL = MainActivity.class.getName() + ":LAST_URL";
  private static final String PREF_URL = "url_";

  MediaPlayer player;
  private LinearLayout tests;
  private LinkedList<String> urls = new LinkedList<>();


  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    log.trace("onCreate()");
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_main);
    tests = (LinearLayout) findViewById(R.id.tests);

    player = new AndrudioMediaPlayer();
    player.setOnPreparedListener(this);


    configureContent();
  }

  private void configureContent() {
    tests.removeAllViews();

    new Test("Pause") {
      @Override
      protected void perform() throws Exception {
        player.pause();
      }
    };

    new Test("Stop") {
      @Override
      protected void perform() throws Exception {
        player.stop();
      }
    };

    SharedPreferences prefs = getPreferences();
    int n = 1;
    urls.clear();
    while (true) {
      String url = prefs.getString(PREF_URL + "_" + n, null);
      if (url == null) break;
      urls.add(url);
      n++;
    }

    for (String url : urls) {
      new PlayTest(url);
    }

    new PlayTest("http://h1.danbrough.org/media/tests/test.ogg");

    // 48khz ogg (a test with an odd sample rate)
    // this tune is driving me insane.
    new PlayTest("http://h1.danbrough.org/media/tests/test48.ogg");

    new PlayTest("http://somafm.com/poptron130.pls");


    // andHow.fm
    new PlayTest("http://www.andhow.fm/playlists/andhow.m3u");

    // WRUV 90.1 fm Burlington VT
    new PlayTest("http://icecast.uvm.edu:8005/wruv_fm_256");

    // Radio One, 91FM in Dunedin
    new PlayTest("http://stream.r1.co.nz:8090/r1-high.mp3");



    // 48khz ogg (a test with an odd sample rate)
    // this tune is driving me insane.
    new PlayTest("http://h1.danbrough.org/media/tests/test48.ogg");
    // music that matters
    new PlayTest("http://live-aacplus-64.kexp.org/kexp64.aac");

    // wellington student radio
    new PlayTest("http://stream.radioactive.fm:8000/ractive");


    // audio/x-scpls
    new PlayTest("http://somafm.com/deepspaceone130.pls");

    // audio/x-mpegurl
    new PlayTest("http://www.listenlive.eu/bbcradio1.m3u");

    // rtsp aac stream
    new PlayTest("rtsp://radionz-wowza.streamguys.com/national/national.stream");

    // mp3 stream
    new PlayTest("http://radionz-ice.streamguys.com/national.mp3");
    // audio/aacp
    new PlayTest("http://radionz-ice.streamguys.com/national");


  }

  @Override
  protected void onStop() {
    log.trace("onStop()");
    super.onStop();
    saveURLS();
  }

  public void saveURLS() {
    log.trace("saveURLS()");
    SharedPreferences.Editor editor = getPreferences().edit();
    int n = 1;
    for (String url : urls) {
      editor.putString(PREF_URL + "_" + n, url);
      n++;
      if (n > 10) break;
    }
    editor.apply();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    log.trace("onCreateOptionsMenu()");
    MenuItem item = menu.add(0, MENU_ADD_URL, 0, "Add URL");
    item.setIcon(R.drawable.ic_playlist_add);
    MenuItemCompat.setShowAsAction(item, MenuItemCompat.SHOW_AS_ACTION_ALWAYS);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    log.trace("onOptionsItemSelected()");
    switch (item.getItemId()) {
      case MENU_ADD_URL:
        showAddURLDialog();
        return true;
    }
    return super.onOptionsItemSelected(item);

  }

  public SharedPreferences getPreferences() {
    return PreferenceManager.getDefaultSharedPreferences(this);
  }

  public void showAddURLDialog() {
    log.debug("showAddURLDialog()");
    AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(this);
    dlgBuilder.setTitle("Add URL");
    FrameLayout layout = new FrameLayout(this);
    layout.setPadding(4, 4, 4, 4);
    final EditText urlText = new EditText(this);
    urlText.setHint("enter a URL to play");
    urlText.setText(getPreferences().getString(PREF_LAST_URL, ""));
    urlText.setSingleLine(true);

    layout.addView(urlText);
    dlgBuilder.setView(layout);
    dlgBuilder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialog, int which) {
        String url = urlText.getText().toString();
        if (url.isEmpty()) {
          log.error("its empty");
          return;
        }
        try {
          play(url);
          configureContent();
        } catch (IOException e) {
          handleError(e);
        }
      }
    });
    dlgBuilder.setNegativeButton(android.R.string.cancel, null);
    Dialog dialog = dlgBuilder.show();
    dialog.setCancelable(false);
    dialog.setCanceledOnTouchOutside(false);


  }

  @Override
  public void onPrepared(MediaPlayer mp) {
    log.trace("onPrepared()");
    mp.start();
  }

  public void play(final String url) throws IOException {
    log.info("play(): {}", url);
    if (url.endsWith(".m3u") ||
        url.endsWith(".asf") ||
        url.endsWith(".pls")) {
      new Thread() {
        @Override
        public void run() {
          try {
            readPlaylist(url);
          } catch (Exception e) {
            handleError(e);
          }
        }
      }.start();
      return;
    }

    getPreferences().edit().putString(PREF_LAST_URL, url).apply();

    urls.remove(url);
    urls.addFirst(url);
    saveURLS();

    invalidateOptionsMenu();

    player.reset();
    player.setDataSource(url);
    player.prepareAsync();
  }

  public void readPlaylist(String url) throws IOException {

    HttpURLConnection conn = (HttpURLConnection) new java.net.URL(url).openConnection();
    conn.setRequestProperty("Accept-Encoding", "gzip");
    conn.connect();
    if (conn.getResponseCode() != HttpURLConnection.HTTP_OK)
      throw new IOException("HTTP_ERROR:" + conn.getResponseCode() + ": " + conn
          .getResponseMessage());

    InputStream input = conn.getInputStream();
    if ("gzip".equals(conn.getContentEncoding())) {
      input = new GZIPInputStream(input);
    }
    BufferedReader reader = new BufferedReader(new InputStreamReader(input));
    String line = null;
    String newUrl = null;
    while ((line = reader.readLine()) != null) {
      if (line.startsWith("File")) {
        int i = line.indexOf('=');
        if (i > 0) {
          newUrl = line.substring(i + 1).trim();
          break;
        }
      }

      if (line.startsWith("http://") || line.startsWith("https://")) {
        newUrl = line.trim();
        break;
      }
    }

    if (newUrl != null) {
      final String nextUrl = newUrl;
      if (newUrl != null) {
        runOnUiThread(new Runnable() {
          @Override
          public void run() {
            try {
              play(nextUrl);
            } catch (IOException e) {
              handleError(e);
            }
          }
        });
      }
    }
    reader.close();

  }

  private void handleError(Exception e) {
    log.error(e.getMessage(), e);
    Toast.makeText(MainActivity.this, e.getMessage(), Toast.LENGTH_SHORT).show();
  }


  private class PlayTest extends Test {
    private final String url;

    public PlayTest(String url) {
      super(url);
      this.url = url;
    }

    @Override
    protected void perform() throws Exception {
      play(url);
    }
  }

  private abstract class Test implements View.OnClickListener {
    protected Test(String label) {
      super();
      Button b = new Button(MainActivity.this);
      b.setText(label);
      b.setOnClickListener(this);
      tests.addView(b);
    }

    @Override
    public void onClick(View v) {
      try {
        perform();
      } catch (Exception e) {
        handleError(e);
      }
    }


    protected abstract void perform() throws Exception;
  }
}
