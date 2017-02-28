
Andrudio Audio Player
=================
A FFMPEG based drop-in replacement for the android [MediaPlayer](http://developer.android.com/reference/android/media/MediaPlayer.html)
for use in audio only applications.

To use this in your own android applications simply add to your maven repositories in your build.gradle:

    repositories {
      maven {
        url "https://h1.danbrough.org/maven"
      }
    }

and add the dependency:

    compile 'danbroid.andrudio:andrudio-library:v2.010'

or you can build from source by doing something like:
    
    git clone git@github.com:danbrough/andrudio
    cd andrudio
    ./ffmpeg/build.sh
    ./gradlew build

You will need to have the android ndk installed and the environment variable `$ANDROID_NDK` set to its location.

For testing there is a demo.apk file ready to install or you can install the demo
from [Google Play](https://play.google.com/store/apps/details?id=danbroid.andrudio.demo)

If you can't compile the native code then you can unpack libs.tar.bz2 in the library directory which contain
the latest binaries.

There is also a command line application for testing the native code:
	see:  `./test/test.sh`

Status
=======

It works. It plays any audio that ffmpeg supports.
Doesn't play .m3u or .pls playlists but you can implement that yoursel by parsing these files from your code.
The demo application does exactly that (but not very well).











    
    
