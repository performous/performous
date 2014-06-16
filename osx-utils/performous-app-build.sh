# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken
if which dylibbundler &> /dev/null; then
    echo "dylibbundler found!"
else
    echo "dylibbundler not found! you need to install it before creating the bundle."
    exit
fi

# first compile performous, build dir shouldn't exist at this stage
rm -rd ./build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=./out/Performous.app/Contents -DENABLE_TOOLS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1 -DSDL2_INCLUDE_DIR=/Library/Frameworks/SDL2.Framework/Headers -DSDL2_LIBRARY=/Library/Frameworks/SDL2.framework -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2/ -DLibXML2_LIBRARY=/opt/local/lib/libxml2.dylib -DLibXML2_INCLUDE_DIR=/opt/local/include/libxml2 -DLibXML++Config_INCLUDE_DIR=/opt/local/lib/libxml++-2.6/include -DGettext_LIBRARY=/opt/local/lib/libgettextlib.dylib  -DGettext_INCLUDE_DIR=/opt/local/include/ -DGlibmmConfig_INCLUDE_DIR=/opt/local/include/glibmm-2.4/glibmm -DGlibConfig_INCLUDE_DIR=/opt/local/include/glib-2.0/glib -DCMAKE_C_COMPILER=/opt/local/bin/gcc-mp-4.8 -DCMAKE_CXX_COMPILER=/opt/local/bin/g++-mp-4.8 -DCMAKE_C_FLAGS="-arch x86_64" -DCMAKE_CXX_FLAGS="-Wno-unused-function -Wno-unused-local-typedefs -Wno-ignored-qualifiers -lstdc++ -arch x86_64 -L/opt/local/lib -lintl -Wl,-framework -Wl,CoreFoundation,-headerpad_max_install_names" -DCMAKE_EXE_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_MODULE_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_SHARED_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_STATIC_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" ../..
make -j2 install

# then create the rest of the app bundle

mkdir out/Performous.app/Contents/MacOS
mkdir out/Performous.app/Contents/Resources
mkdir out/Performous.app/Contents/Frameworks

mv out/Performous.app/Contents/bin/* out/Performous.app/Contents/MacOS/

cp ../resources/performous-launcher out/Performous.app/Contents/MacOS/
cp ../resources/performous.icns out/Performous.app/Contents/Resources
cp ../resources/Info.plist out/Performous.app/Contents/
cp -R ../resources/etc out/Performous.app/Contents/Resources

cp -R /Library/Frameworks/SDL2.framework out/Performous.app/Contents/Frameworks/SDL2.framework

dylibbundler -od -b -x ./out/Performous.app/Contents/MacOS/performous -d ./out/Performous.app/Contents/libs/

# then build the disk image
ln -sf /Applications out/Applications
/usr/bin/hdiutil create -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
/usr/bin/hdiutil convert RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
rm -f RWPerformous.dmg

cd ..