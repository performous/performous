#!/bin/bash

# sudo port install autoconf automake libtool subversion help2man libsdl cairo librsvg portaudio libxm2 boost +complete imagemagick ffmpeg +avfilter
# ./autogen.sh
# portAudioV19_CFLAGS=-I/opt/local/include portAudioV19_LIBS=-L/opt/local/lib CPPFLAGS=-I/opt/local/include LDFLAGS=-L/opt/local/lib ffmpeg_CFLAGS=-I/opt/local/include ffmpeg_LIBS=-L/opt/local/lib ./configure --prefix=/apps/ultrastar-ng --enable-libda-pa19 --with-audio=ffmpeg --with-video=ffmpeg
# make

mkdir -p Ultrastar-ng.app/Contents/{MacOS,Resources}
cd Ultrastar-ng.app/Contents/

echo '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>usng</string>
	<key>CFBundleIconFile</key>
	<string>ultrastar-ng.icns</string>
	<key>CFBundleIdentifier</key>
	<string>net.sourceforge.ultrastar-ng</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleVersion</key>
	<string>VERSION</string>
</dict>
</plist>
' > Info.plist

sed -i.bak -e "s/VERSION/0.2.1/" Info.plist
rm Info.plist.bak

cp ../../osx-utils/ultrastar-ng.icns Resources/
cp ../../osx-utils/usng-mac_script MacOS/usng
chmod +x MacOS/usng

cd Resources
mkdir -p ultrastar-ng/{bin,lib,share}
mkdir ultrastar-ng/share/ultrastar-ng
cp ../../../src/.libs/ultrastarng ultrastar-ng/bin/
cp ../../../audio/.libs/{libdausng.0.0.0.dylib,libdausng.0.dylib,libdausng.a,libdausng.dylib,libdausng.la} ultrastar-ng/lib/
cp -R ../../../themes ultrastar-ng/share/ultrastar-ng/
