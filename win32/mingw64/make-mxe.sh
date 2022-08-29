#!/bin/sh
echo "installing git in case it hasn't been done yet"
sudo apt-get install git-core
git clone -b master https://github.com/mxe/mxe.git
cp settings.mk ./mxe/settings.mk
cd mxe
make
cd ..
echo "mxe has been build please run the make-performous script"
