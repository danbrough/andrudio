#!/bin/bash

cd `dirname $0`
TARFILE=libs.tar.bz2

rm $TARFILE 2> /dev/null
tar cvjpf $TARFILE libs

