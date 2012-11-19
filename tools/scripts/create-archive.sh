#!/usr/bin/env sh

NAME=performous-`git describe --tags`
FILE=$NAME.tar.gz
OUTDIR=`pwd`

cd `dirname $0`/../..

git archive --prefix="$NAME/" -o "$OUTDIR/$FILE" HEAD

