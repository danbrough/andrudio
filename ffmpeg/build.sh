#!/usr/bin/env bash

cd `dirname $0`
ROOT=`pwd`
. common.sh



build(){
  ABI="$1"
  log_info "build: $ABI on $HOST_SYSTEM with NDK: $NDK"
  . ${ROOT}/${ABI}.sh

  export PATH=$TOOLCHAIN/bin:$PATH
  export CC="${CROSS_PREFIX}gcc"
  export CXX=${CROSS_PREFIX}g++
  export LD=${CROSS_PREFIX}ld
  export AR=${CROSS_PREFIX}ar
  export STRIP=${CROSS_PREFIX}strip
  export FLAGS="$FLAGS"
  DEST="${ROOT}/build/$ABI"

  rm -rf $DEST 2> /dev/null
  mkdir -p $DEST || exit 1
  rm -rf $ROOT/libs/$ABI

  log_info "compiling ffmpeg to $DEST with $FLAGS"
  log_info "ECFLAGS: $ECFLAGS"
  log_info "ELDFLAGS: $ELDFLAGS"


  cd $FFMPEG_SRC
  log_info "$ABI: cleaning source"
  ( git clean -xdf && git reset --hard ) > /dev/null 2>&1
  git checkout $FFMPEG_VERSION > /dev/null 2>&1 || exit 1
  ./configure $FLAGS --extra-cflags="$ECFLAGS" --extra-ldflags="$ELDFLAGS" --prefix="$DEST" \
    | tee $DEST/configuration.txt  > ${DEST}/configure.log

  if [ $PIPESTATUS != 0 ]; then
    cat ${DEST}/configure.log
    log_err "$ABI: configure failed"
  fi

  log_info "$ABI: starting compile"
  make -j4 > ${DEST}/build.log 2>&1 || log_err "build failed see ${DEST}/build.log"
  make install > /dev/null 2>&1 || log_err "make install failed"
  log_info "$ABI: done"
  log_info ""
  mkdir -p $ROOT/libs/$ABI || exit 1
  mv ${DEST}/lib/*.so  ${ROOT}/libs/$ABI || exit 1
  if [ "$ABI" == "armeabi" ]; then
    rsync -av  --delete $DEST/include/ $ROOT/include/ > /dev/null 2>&1
  fi

}

build_all(){
  for ABI in  x86 x86_64 armeabi armeabi-v7a arm64-v8a; do
    build $ABI HOST: $HOST_SYSTEM
  done
}


if [ $# == 0 ]; then
  log_info "building all"
  build_all
elif [ "$1" == "clean" ]; then
  clean
else
  for ABI in $@; do
    build $ABI
  done
fi