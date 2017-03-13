#!/usr/bin/env bash

cd `dirname $0` && cd ffmpeg.git

./configure --help

for setting in `./configure  --help | awk '/--list/ {print $1}'`; do
	echo '################' $setting '########################'
	./configure $setting
	echo
	echo
done


