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
ABI="armeabi"

export OPENSSL_BUILD=$BUILD/openssl
if (( $DSSL )); then
  log should build ssl
  cd $OPENSSL 
  ../setenv-android.sh
  perl -pi -e 's/install: all install_docs install_sw/install: install_docs install_sw/g' Makefile.org || exit 1
  
  rm -rf $OPENSSL_BUILD && mkdir -p $OPENSSL_BUILD 2> /dev/null
  ./config shared no-ssl2 no-ssl3 no-comp no-hw no-engine --openssldir=$OPENSSL_BUILD || exit 1
  make depend || exit 1
  make all || exit 1
  make install
fi


ECFLAGS=""
ELDFLAGS=""
SYSROOT="${NDK}/platforms/android-8/arch-arm"
TOOLCHAIN=${NDK}/toolchains/${CROSS_PREFIX}4.9/prebuilt/${HOST_SYSTEM}

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

export FLAGS="--arch=arm"
export FLAGS="$FLAGS --enable-cross-compile --cross-prefix=$CROSS_PREFIX"
export FLAGS="$FLAGS --enable-shared --disable-symver --disable-static"
export FLAGS="$FLAGS --target-os=android --sysroot=$SYSROOT"

source ../common_flags.sh

export FLAGS="$FLAGS --enable-openssl"
export ECFLAGS="$ECFLAGS -I$OPENSSL_BUILD/include -I$OPENSSL_BUILD/include/openssl"
export ELDFLAGS="$ELDFLAGS -L$OPENSSL_BUILD/lib "


./configure $FLAGS --extra-cflags="$ECFLAGS" --extra-ldflags="$ELDFLAGS" --prefix="$DEST" | tee $DEST/configuration.txt
[ $PIPESTATUS == 0 ] || exit 1
cat $DEST/configuration.txt
make clean
make -j4 || exit 1
make install || exit 1
rm -rf $JNIDIR/$ABI
mv $DEST $JNIDIR/
log build complete


 

  

