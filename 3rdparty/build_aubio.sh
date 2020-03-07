#!/bin/bash
echo export CFLAGS=${3}
export CFLAGS=${3}

buildCmd="'${BASH_SOURCE%/*}/aubio/waf' --verbose configure build install $1 --enable-fftw3f --disable-sndfile --disable-avcodec --disable-double --disable-samplerate --disable-docs --disable-wavread --disable-wavwrite --disable-tests --notests --disable-examples --disable-apple-audio --disable-fat --disable-jack --prefix='$2' --out=$2 --top='${BASH_SOURCE%/*}/aubio/'"

mkdir -p "$2"
echo ${buildCmd} && eval ${buildCmd}