#!/bin/bash

cd `dirname $0`

ROOT=`pwd`
BUILD=$ROOT/build_libav
LIBAV=$BUILD/libav
NDK=/home/dan/sdk/android-ndk
JNIDIR=$ROOT/jni/libav/libav
TAG=v11.3
#TAG=master

./clean.sh
[ ! -d $JNIDIR ] && mkdir -p $JNIDIR

log(){
  printf '\x1b[1;32m### %s\x1b[0m\n' "$*"
}


setup_libav(){
  log setup_libav
  if [ ! -d $LIBAV ]; then
    log setting up $LIBAV 
    git clone git://git.libav.org/libav.git $LIBAV    
  fi
  cd $LIBAV
  git clean -xdf > /dev/null 2>&1
  git checkout $TAG
  git reset --hard > /dev/null 2>&1
}

source $BUILD/build_x86_64.sh || exit 1
source $BUILD/build_arm.sh || exit 1
source $BUILD/build_armv7.sh || exit 1
source $BUILD/build_x86.sh || exit 1
cd $ROOT && ndk-build









