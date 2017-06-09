# This Makefile allows you to build on osx, using homebrew rather than
# MacPorts, for reasons that will be obvious to anybody who uses
# homebrew and has tried to use *both* homebrew and MacPorts on the
# same box before.
#
# Using it is fairly straightforward:
#
# Step 1: install homebrew.
#
# Step 2: Run make
# ```
# make -f osx.mk all
# ```

all : build dependencies
	cd build && cmake \
		-D Crypto_INCLUDE_DIR=/usr/local/Cellar/openssl/1.0.2h_1/include/openssl/ \
		-D Ssl_INCLUDE_DIR=/usr/local/Cellar/openssl/1.0.2h_1/include/openssl/ ..
	cd build && make

clean:
	rm -rf build

build:
	mkdir $@

dependencies: .install-cmake .install-libsdl2 .install-help2man .install-libepoxy .install-cairo .install-librsvg .install-portaudio .install-portmidi .install-opencv .install-dylibbundler .install-libxml++ .install-jsoncpp .install-openssl .install-libressl .install-boost .install-cppnetlib

.install-cppnetlib:
	brew install earlye/homebrew-boneyard/cpp-netlib
	touch $@

.install-boost:
	if brew ls --versions boost > /dev/null; then echo "boost is installed" ; else brew install boost; fi
	touch $@

.install-libressl:
	brew install libressl
	touch $@

.install-openssl:
	brew install openssl
	touch $@

.install-jsoncpp:
	brew install jsoncpp
	touch $@

.install-libxml++:
	brew install libxml++
	touch $@

.install-cmake:
	brew install cmake
	touch $@

.install-libsdl2 :
	brew install sdl2
	touch $@

.install-help2man:
	brew install help2man
	touch $@

.install-libepoxy:
	brew install libepoxy
	touch $@

.install-cairo:
	brew install cairo
	touch $@

.install-librsvg:
	brew install librsvg
	touch $@

.install-portaudio:
	brew install portaudio
	touch $@

.install-portmidi:
	brew install portmidi
	touch $@

.install-opencv:
	brew tap homebrew/science
	brew install opencv3
	touch $@

.install-dylibbundler:
	brew install dylibbundler
	touch $@
