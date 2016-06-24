#!/bin/bash


log(){
  printf '\x1b[1;32m### %s\x1b[0m\n' "$*"
}

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

[ -f local.env ] && source local.env

OPENSSL=$BUILD/openssl.git

export SRC=$BUILD/ffmpeg.git
export TAG=n3.0
#export TAG=n3.0.1
#export TAG=n3.1-dev


if [ ! -z "$CUSTOM_TAG" ]; then
  export TAG="$CUSTOM_TAG"
fi


setup_source(){
  log setup_source
  if [ ! -d $SRC ]; then
    log setting up $SRC 
    git clone git://source.ffmpeg.org/ffmpeg.git $SRC
  fi
  cd $SRC
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
