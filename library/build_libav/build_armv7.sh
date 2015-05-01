#!/bin/bash

log building arm

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

log HOST_SYSTEM $HOST_SYSTEM

CROSS_PREFIX=arm-linux-androideabi-
ABI="armeabi-v7a"

ECFLAGS="-mfloat-abi=softfp -Wno-asm-operand-widths"
ELDFLAGS="-Wl,--fix-cortex-a8"
TOOLCHAIN=/tmp/mediaplayer
#SYSROOT=$TOOLCHAIN/sysroot/
SYSROOT="${NDK}/platforms/android-8/arch-arm"
TOOLCHAIN=${NDK}/toolchains/${CROSS_PREFIX}4.9/prebuilt/${HOST_SYSTEM}


#$NDK/build/tools/make-standalone-toolchain.sh --toolchain=${CROSS_PREFIX}4.9 --platform=android-9 \
#  --system=$HOST_SYSTEM --install-dir=$TOOLCHAIN

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

export FLAGS="--arch=armv7-a"
export FLAGS="$FLAGS --enable-cross-compile --cross-prefix=$CROSS_PREFIX"
export FLAGS="$FLAGS --enable-shared --disable-symver --disable-static"
export FLAGS="$FLAGS --target-os=android --sysroot=$SYSROOT"

source ../common_flags.sh

log $FLAGS --extra-cflags="$ECFLAGS" --extra-ldflags="$ELDFLAGS" --prefix="$DEST"
./configure $FLAGS --extra-cflags="$ECFLAGS" --extra-ldflags="$ELDFLAGS" --prefix="$DEST" | tee $DEST/configuration.txt
[ $PIPESTATUS == 0 ] || exit 1
cat $DEST/configuration.txt
make clean
make -j4 || exit 1
make install || exit 1
rm -rf $JNIDIR/$ABI
mv $DEST $JNIDIR/
log build complete


 

  

