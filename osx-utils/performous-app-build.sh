#!/bin/bash

# mkdir build
# cd build
# cmake ..
# make

export VER=$(cd .. ; svn info | grep Revision | sed -e 's/Revision: //')

mkdir -p Performous.app/Contents/{MacOS,Resources}
cp ../osx-utils/performous.icns Performous.app/Contents/Resources/
cp ../osx-utils/performous-mac_script Performous.app/Contents/MacOS/performous
chmod +x Performous.app/Contents/MacOS/performous
cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/Performous.app/Contents/Resources/performous
make install
cd Performous.app/Contents/

echo '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>performous</string>
	<key>CFBundleIconFile</key>
	<string>performous.icns</string>
	<key>CFBundleIdentifier</key>
	<string>net.sourceforge.performous</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleVersion</key>
	<string>VERSION</string>
</dict>
</plist>
' > Info.plist

sed -i.bak -e "s/VERSION/0.3.0 \(r$VER\)/" Info.plist
rm Info.plist.bak
