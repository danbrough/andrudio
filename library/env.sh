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

SSL=0
if grep SSL jni/Android.mk | grep 1 > /dev/null; then
  log "using openssl as found SSL := 1 in Android.mk"
  SSL=1
fi
export SSL

if [ -z $ANDROID_NDK ]; then
  log please set ANDROID_NDK to the root of your android ndk installation
  exit 1
fi

export ROOT=`pwd`
export BUILD=$ROOT/build_native
export NDK=$ANDROID_NDK
export PATH=$NDK:$PATH
export JNIDIR=$ROOT/jni/native/native

OPENSSL=$BUILD/openssl.git

if (( $FFMPEG )); then
  export LIBAV=$BUILD/ffmpeg.git
  export TAG=n2.7.2
  #export TAG=master
else
  export LIBAV=$BUILD/libav.git
  export TAG=v11.4
  #export TAG=master
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
  git clean -xdf && git reset --hard > /dev/null 2>&1
  git checkout $TAG  
  
  if [ $SSL == "1" ]; then
    log setting up openssl source
    if [ ! -d $OPENSSL ]; then
      git clone git://git.openssl.org/openssl.git $OPENSSL || exit 1      
    fi
    cd $OPENSSL
    git clean -xdf && git reset --hard > /dev/null 2>&1    
    git checkout OpenSSL_1_0_2a || exit 1       
  fi
}
