
Andrudio Audio Player
=================
A LibAV/FFMPEG based drop-in replacement for the android [MediaPlayer](http://developer.android.com/reference/android/media/MediaPlayer.html)
    
    git clone git@github.com:danbrough/andrudio
    cd andrudio
    ./library/build.sh
    gradle build

You will need to have the android ndk installed and the environment variable `$ANDROID_NDK_HOME` set to its location.
The gradle build scripts are for gradle 2.3.

For testing there is a ./demo/build/outputs/apk/demo-debug.apk file ready to go.

There is also a command line application for testing the native code:
	see:  `./library/test/test.sh`

To use libav instead of ffmpeg set `FFMPEG := 0` in `library/jni/Android.mk`.

=======
Status
------

Basics are all working.
Some nasty bugs leading to lockups.






    
    
