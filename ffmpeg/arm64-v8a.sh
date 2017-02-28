#!/usr/bin/env bash



CROSS_PREFIX=aarch64-linux-android-
SYSROOT="${NDK}/platforms/android-21/arch-arm64"
TOOLCHAIN=${NDK}/toolchains/aarch64-linux-android-4.9/prebuilt/${HOST_SYSTEM}

export FLAGS="--arch=aarch64"
. ${ROOT}/flags.sh



