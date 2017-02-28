#!/usr/bin/env bash


FFMPEG_SRC="${ROOT}/ffmpeg.git"
FFMPEG_VERSION=n3.2.4

OS=`uname`
HOST_ARCH=`uname -m`
if [ $OS == 'Linux' ]; then
  export HOST_SYSTEM=linux-$HOST_ARCH
elif [ $OS == 'Darwin' ]; then
  export HOST_SYSTEM=darwin-$HOST_ARCH
fi

log_info(){
        printf '\x1b[1;32m### %s\x1b[0m\n' "$*"
}

log_err(){
        printf '\x1b[1;33m### %s\x1b[0m\n' "$*"
        exit 1
}

if [ -z "$ANDROID_NDK" ]; then
  if [ -z "$NDK" ]; then
    log_err "Neither NDK or ANDROID_NDK is set"
  fi
else
  export NDK="$ANDROID_NDK"
fi

if [ ! -d $FFMPEG_SRC ]; then
  log_info "download ffmpeg source"
  git clone git://source.ffmpeg.org/ffmpeg.git $FFMPEG_SRC || exit 1
fi

clean(){
  log_info "clean()"
  rm -rf ${ROOT}/libs/*
  rm -rf ${ROOT}/build
}

[ ! -d $ROOT/libs ] && mkdir -p ${ROOT}/libs
[ ! -d $ROOT/include ] && mkdir -p ${ROOT}/include