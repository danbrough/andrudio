#!/bin/bash

###########################################################################
# This test program compiles and runs the native code used in this library.
# It uses libao to play audio. See: http://xiph.org/ao/ and yoiu will need
# ffmpeg to be installed on your system.
#
# The environment variable MEMCHECK can have the value 1 or 2 to enable 
# memory checking. (Valgrind needs to be installed)
#
# The environment variable LIBAO can be set to 0 to disable audio output 
# via libao (program will decode audio only)
#
# There are various keys you can use. Have a look at main.c to see the
# control loop
###########################################################################


cd `dirname $0`

ROOT=../ffmpeg
EXE=./andrudiotest

source ${ROOT}/common.sh

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

SRC_DIR=../lib/src/main/native



gcc -g -O0 -DUSE_COLOR=1   $EXTRA_FLAGS main.c ${SRC_DIR}/player_thread.c \
  ${SRC_DIR}/audioplayer.c  -I${SRC_DIR}  -o $EXE  \
  -lz -lbz2 -lc -lm -lavutil  ${LDFLAGS} -lavcodec -lavformat -lavresample -lpthread || exit 1

WRAPPER=""

if [ "$MEMCHECK" == "1" ]; then
	WRAPPER="valgrind --leak-check=yes" 
elif [ "$MEMCHECK" == "2" ]; then
	WRAPPER="valgrind -v --leak-check=yes --leak-check=full --show-leak-kinds=all "
fi

$WRAPPER $EXE "$URL"


