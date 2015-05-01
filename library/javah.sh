cd `dirname $0`
cd src
javah -classpath .:$ANDROID_SDK/platforms/android-8/android.jar  -d ../jni/player/ danbroid.andrudio.LibAndrudio

