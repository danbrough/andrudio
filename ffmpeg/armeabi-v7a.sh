#!/usr/bin/env bash


CROSS_PREFIX=arm-linux-androideabi-

SYSROOT="${NDK}/platforms/android-9/arch-arm"
TOOLCHAIN=${NDK}/toolchains/${CROSS_PREFIX}4.9/prebuilt/${HOST_SYSTEM}

FLAGS="--arch=armv7-a"
. ${ROOT}/flags.sh

ECFLAGS="-mfloat-abi=softfp -Wno-asm-operand-widths"
ELDFLAGS="-Wl,--fix-cortex-a8"
