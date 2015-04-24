
Android Audio Player
=================
An LibAV based drop-in replacement for the android [MediaPlayer](http://developer.android.com/reference/android/media/MediaPlayer.html)
    
    git clone git@github.com:danbrough/androidaudioplayer.git
    cd androidaudioplayer
    ./library/build.sh
    gradle build

You will need to have the android ndk installed and the environment variable `$ANDROID_NDK_HOME` set to its location.
The gradle build scripts are for gradle 2.3.

For testing there is a ./demo/demo.apk file ready to go.

=======
Status
------

Basics are all working.
Some nasty bugs leading to lockups.






    
    
