#!/bin/bash

###########################################################################
# This test program compiles and runs the native code used in this library.
# It requires that libav and libao are installed
# (and you are running linux)
# see: https://libav.org/ and http://xiph.org/ao/
#
# The environment variable MEMCHECK can have the value 1 or 2 to enable 
# memory checking. (Valgrind needs to be installed)
#
# The environment variable LIBAO can be set to 0 to disable audio output 
# via libao (program will decode audio only)
###########################################################################


cd `dirname $0` 
TEST_ROOT=`pwd`

export ANDROID_NDK=/tmp/notneeded
cd .. && . env.sh



if (( $FFMPEG )); then
log compiling ffmpeg
else
log compiling libav
fi

export BUILD=$BUILD/test
export EXE=$BUILD/playertest

if [ ! -d $BUILD ]; then   
  setup_source
  cd $SRC
  source ../common_flags.sh
  ./configure $FLAGS --prefix=$BUILD --disable-shared --enable-static || exit 1
  make -j4 || exit 1
  make install || exit 1
fi

cd $ROOT/test

URL=""

if [ "$1" != "" ]; then
	URL="$1"
elif [ "$AUDIO_URL" != "" ]; then
	URL="$AUDIO_URL"
fi

EXTRA_FLAGS=""
if [ "$LIBAO" == "0" ]; then
  EXTRA_FLAGS="-DDISABLE_AUDIO"
else
  LDFLAGS="-lao"
fi

gcc -g -O0 -DUSE_COLOR=1 $EXTRA_FLAGS main.c ../jni/player/player_thread.c \
  ../jni/player/audioplayer.c  -I../jni/player/  -o $EXE  $BUILD/lib/lib*.a  \
  $LDFLAGS -lz -lbz2 -lc -lm -lavutil  -lavcodec -lavformat -lavresample -lpthread \
 -I${BUILD}/include -L${BUILD}/lib || exit 1  

WRAPPER=""

if [ "$MEMCHECK" == "1" ]; then
	WRAPPER="valgrind --leak-check=yes" 
elif [ "$MEMCHECK" == "2" ]; then
	WRAPPER="valgrind -v --leak-check=yes --leak-check=full --show-leak-kinds=all "
fi


$WRAPPER $EXE "$URL"


