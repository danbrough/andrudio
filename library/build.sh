#!/bin/bash

cd `dirname $0`

source env.sh

./clean.sh
[ ! -d $JNIDIR ] && mkdir -p $JNIDIR

source $BUILD/build_arm.sh || exit 1
source $BUILD/build_armv7.sh || exit 1
source $BUILD/build_arm64_v8a.sh || exit 1
source $BUILD/build_x86_64.sh || exit 1
source $BUILD/build_mips.sh || exit 1
source $BUILD/build_x86.sh || exit 1

#cd $ROOT && ndk-build && ./tarlibs.sh









