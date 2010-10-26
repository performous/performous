# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken
if which dylibbundler &> /dev/null; then
    echo "dylibbundler found!"
else
    echo "dylibbundler not found! you need to install it before creating the bundle."
    exit
fi

# first compile performous, build dir shouldn't exist at this stage
mkdir build
cd build
cmake ../../ -DCMAKE_INSTALL_PREFIX=./out/Performous.app/Contents -DENABLE_TOOLS=OFF
make install

# then create the rest of the app bundle

mkdir out/Performous.app/Contents/MacOS
mkdir out/Performous.app/Contents/Resources
mkdir out/Performous.app/Contents/Frameworks

mv out/Performous.app/Contents/bin/* out/Performous.app/Contents/MacOS/

cp ../resources/performous-launcher out/Performous.app/Contents/MacOS/
cp ../resources/performous.icns out/Performous.app/Contents/Resources
cp ../resources/Info.plist out/Performous.app/Contents/
cp -R ../resources/etc out/Performous.app/Contents/Resources

cp -R /Library/Frameworks/SDL.framework out/Performous.app/Contents/Frameworks/SDL.framework

dylibbundler -od -b -x ./out/Performous.app/Contents/MacOS/performous -d ./out/Performous.app/Contents/libs/

# then build the disk image
ln -sf /Applications out/Applications
/usr/bin/hdiutil create -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
/usr/bin/hdiutil convert RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
rm -f RWPerformous.dmg

cd ..
