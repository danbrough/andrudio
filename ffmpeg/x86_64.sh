#!/usr/bin/env bash

CROSS_PREFIX=x86_64-linux-android-
ABI="x86_64"
SYSROOT="${NDK}/platforms/android-21/arch-x86_64"
TOOLCHAIN=${NDK}/toolchains/x86_64-4.9/prebuilt/${HOST_SYSTEM}

FLAGS="--arch=$ABI"
. ${ROOT}/flags.sh


