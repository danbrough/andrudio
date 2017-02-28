#!/usr/bin/env bash

CROSS_PREFIX=arm-linux-androideabi-
SYSROOT="${NDK}/platforms/android-9/arch-arm"
TOOLCHAIN=${NDK}/toolchains/${CROSS_PREFIX}4.9/prebuilt/${HOST_SYSTEM}

FLAGS="--arch=$ABI"
. ${ROOT}/flags.sh
