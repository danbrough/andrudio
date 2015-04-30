#!/bin/bash

###########################################################################
# This test program compiles and runs the native code used in this library.
# It requires that libav and libao are installed
# (and you are running linux)
# see: https://libav.org/ and http://xiph.org/ao/
###########################################################################

cd `dirname $0` && cd ..
ROOT=`pwd`

TEST=$ROOT/test
BUILD=$TEST/libav
SRC=$ROOT/build_libav/libav

if [ ! -d $SRC ]; then
  git clone git://git.libav.org/libav.git $SRC || exit 1
  cd $SRC
fi


if [ ! -d $BUILD ]; then
  cd $SRC
  git clean -xdf
  git checkout v11.3
  git clean -xdf
  git reset --hard
  source ../common_flags.sh
  ./configure $FLAGS --prefix=$BUILD --enable-shared --disable-static || exit 1
  make -j4 || exit 1
  make install || exit 1
fi

cd $TEST

URL=""

if [ "$1" != "" ]; then
	URL="$1"
elif [ "$AUDIO_URL" != "" ]; then
	URL="$AUDIO_URL"
fi

EXTRA_FLAGS=""
if [ ! -z $DISABLE_AUDIO ]; then
  EXTRA_FLAGS="-DDISABLE_AUDIO"
fi

gcc -g -O0 -DUSE_COLOR=1 $EXTRA_FLAGS main.c ../jni/player/packet_queue.c ../jni/player/read_thread.c \
	../jni/player/play_thread.c  ../jni/player/audioplayer.c  -I../jni/player/  -o /tmp/playertest \
   -lavutil -lavformat -lavcodec -lavresample -lao -lpthread -lc -lm -Ilibav/include -Llibav/lib || exit 1

WRAPPER=""

if [ "$MEMCHECK" == "1" ]; then
	WRAPPER="valgrind --leak-check=yes" 
elif [ "$MEMCHECK" == "2" ]; then
	WRAPPER="valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all "
fi

$WRAPPER /tmp/playertest $URL


