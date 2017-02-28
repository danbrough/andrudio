#!/usr/bin/env bash

CROSS_PREFIX=i686-linux-android-
ABI="x86"
SYSROOT="${NDK}/platforms/android-9/arch-x86"
TOOLCHAIN=${NDK}/toolchains/x86-4.9/prebuilt/${HOST_SYSTEM}

FLAGS="--arch=$ABI"
. ${ROOT}/flags.sh
FLAGS="$FLAGS --disable-asm"
