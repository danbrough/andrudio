#!/bin/bash


log(){
  printf '\x1b[1;32m### %s\x1b[0m\n' "$*"
}


FFMPEG=0
if grep FFMPEG jni/Android.mk | grep 1 > /dev/null; then
  log "using ffmpeg as found FFMPEG := 1 in Android.mk"
  FFMPEG=1
fi
export FFMPEG


export ROOT=`pwd`
export BUILD=$ROOT/build_native
export NDK=$ANDROID_NDK
export PATH=$NDK:$PATH
export JNIDIR=$ROOT/jni/native/native

if (( $FFMPEG )); then
  export LIBAV=$BUILD/ffmpeg.git
  export TAG=n2.6.2
else
  export LIBAV=$BUILD/libav.git
  export TAG=v11.3
fi


setup_source(){
  log setup_source
  if [ ! -d $LIBAV ]; then
    log setting up $LIBAV 
    if (( $FFMPEG )); then
      git clone git://source.ffmpeg.org/ffmpeg.git $LIBAV
    else
      git clone git://git.libav.org/libav.git $LIBAV
    fi    
  fi
  cd $LIBAV
  git clean -xdf > /dev/null 2>&1
  git checkout $TAG
  git reset --hard > /dev/null 2>&1
}
