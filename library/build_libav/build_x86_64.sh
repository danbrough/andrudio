#!/bin/bash

log building x86_64

setup_source
cd $BUILD

# Detect OS
OS=`uname`
HOST_ARCH=`uname -m`
if [ $OS == 'Linux' ]; then
  export HOST_SYSTEM=linux-$HOST_ARCH
elif [ $OS == 'Darwin' ]; then
  export HOST_SYSTEM=darwin-$HOST_ARCH
fi


CROSS_PREFIX=x86_64-linux-android-
ABI="x86_64"

ECFLAGS=""
ELDFLAGS=""
SYSROOT="${NDK}/platforms/android-21/arch-x86_64"
TOOLCHAIN=${NDK}/toolchains/x86_64-4.9/prebuilt/${HOST_SYSTEM}

export PATH=$TOOLCHAIN/bin:$PATH
export CC="${CROSS_PREFIX}gcc"
export CXX=${CROSS_PREFIX}g++
export LD=${CROSS_PREFIX}ld
export AR=${CROSS_PREFIX}ar
export STRIP=${CROSS_PREFIX}strip

DEST=$BUILD/build
cd $LIBAV


DEST="$DEST/$ABI"
mkdir -p $DEST

export FLAGS="--arch=x86_64"
export FLAGS="$FLAGS --enable-cross-compile --cross-prefix=$CROSS_PREFIX"
export FLAGS="$FLAGS --enable-shared --disable-symver --disable-static"
export FLAGS="$FLAGS --target-os=android --sysroot=$SYSROOT"

source ../common_flags.sh

./configure $FLAGS --extra-cflags="$ECFLAGS" --extra-ldflags="$ELDFLAGS" --prefix="$DEST"  | tee $DEST/configuration.txt
[ $PIPESTATUS == 0 ] || exit 1
cat $DEST/configuration.txt
make clean
make -j4 || exit 1
make install || exit 1
rm -rf $JNIDIR/$ABI 
mv $DEST $JNIDIR/

log build complete


 

  

