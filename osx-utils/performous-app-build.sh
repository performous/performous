set -o errexit
# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken
if which dylibbundler &> /dev/null; then
    echo "dylibbundler found!"
else
    echo "dylibbundler not found! you need to install it before creating the bundle."
    exit
fi

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
CURRDIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

# first compile performous, build dir shouldn't exist at this stage

# define Bundle structure
TEMPDIR="$CURRDIR/out/Performous.app/Contents"
RESDIR="$TEMPDIR/Resources"
LIBDIR="$RESDIR/lib"
LOCALEDIR="$RESDIR/Locales"
FRAMEWORKDIR="$RESDIR/Frameworks"
BINDIR="$TEMPDIR/MacOS"
ETCDIR="$RESDIR/etc"

rm -rf "$TEMPDIR"
mkdir -p "$TEMPDIR"

rm -rf ./build
mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX=$TEMPDIR -DENABLE_TOOLS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=1 -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 -DFontconfig_INCLUDE_DIR=/opt/local/include/fontconfig -DPng_INCLUDE_DIR=/opt/local/include/libpng -DAVCodec_INCLUDE_DIR=/opt/local/include/libavcodec -DOpenGL_GL_LIBRARY=/System/Library/Frameworks/OpenGL.framework -DOpenGL_GLU_LIBRARY=/System/Library/Frameworks/OpenGL.framework -DOpenGL_INCLUDE_DIR=/System/Library/Frameworks/OpenGL.framework/Headers -DAVFormat_INCLUDE_DIR=/opt/local/include/libavformat -DSWScale_INCLUDE_DIR=/opt/local/include/libswscale -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2/ -DLibXML2_LIBRARY=/opt/local/lib/libxml2.dylib -DLibXML2_INCLUDE_DIR=/opt/local/include/libxml2 -DLibXML++Config_INCLUDE_DIR=/opt/local/lib/libxml++-2.6/include -DGettext_LIBRARY=/opt/local/lib/libgettextlib.dylib  -DGettext_INCLUDE_DIR=/opt/local/include/ -DGlibmmConfig_INCLUDE_DIR=/opt/local/lib/glibmm-2.4/include -DGlibConfig_INCLUDE_DIR=/opt/local/lib/glib-2.0/include -DCMAKE_C_COMPILER=/opt/local/bin/gcc-mp-4.8 -DCMAKE_CXX_COMPILER=/opt/local/bin/g++-mp-4.8 -DCMAKE_C_FLAGS="-arch x86_64" -DSHARE_INSTALL=Resources -DLOCALE_DIR=Resources/Locales -DCMAKE_CXX_FLAGS="-Wno-unused-function -Wno-unused-local-typedefs -Wno-ignored-qualifiers -lstdc++ -arch x86_64 -L/opt/local/lib -lintl -Wl,-framework -Wl,CoreFoundation,-headerpad_max_install_names" -DCMAKE_EXE_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_MODULE_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_SHARED_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" -DCMAKE_STATIC_LINKER_FLAGS="-ldl -lstdc++ -arch x86_64" ../..
make -j2 install

# then create the rest of the app bundle

mv "$TEMPDIR/bin" "$BINDIR"

cp ../resources/performous-launcher "$BINDIR"
cp ../resources/performous.icns "$RESDIR"
cp ../resources/Info.plist "$TEMPDIR"

mkdir -p $FRAMEWORKDIR
mkdir -p $LIBDIR

dylibbundler -od -b -x "$BINDIR/performous" -d "$LIBDIR" -p @executable_path/../Resources/lib/

cp -av /opt/local/lib/pango $LIBDIR/

OLDPREFIX=/opt/local
NEWPREFIX=$LIBDIR

mkdir -p $ETCDIR/fonts
cp -av /opt/local/etc/fonts $ETCDIR
cp -av /opt/local/etc/pango $ETCDIR
cd $ETCDIR/pango

sed -i '' -e "s|$OLDPREFIX\/etc\/pango\/|\.\.\/Resources\/etc\/pango\/|g" pangorc
sed -i '' -e "s|$OLDPREFIX|\.\.\/\.\.\/\.\.\/\.\.|g" pango.modules

cd $ETCDIR/fonts
sed -i '' -e 's|\/opt\/local/share|\.\.\/\.\.\/\.\.\/Resources|g' fonts.conf
sed -i '' -e 's|\/opt\/local/var/cache|\~\/\.cache|g' fonts.conf
sed -i '' -e 's|\<\!-- Font directory list --\>|\<\!-- Font directory list --\>\
    <dir>\.\.\/\.\.\/pixmaps</dir>|g' fonts.conf

cd $LIBDIR/pango/
cd `find . -type d -maxdepth 1 -mindepth 1`
cd `find . -type d -maxdepth 1 -mindepth 1` 

for if in `pwd`/*.so
do
dylibbundler -x "$if" \
-p @executable_path/../Resources/lib/ \
-d "$LIBDIR"
done 


cd "$CURRDIR"
# then build the disk image
ln -sf /Applications "${CURRDIR}/out/Applications"
/usr/bin/hdiutil create -ov -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
/usr/bin/hdiutil convert -ov RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
rm -f RWPerformous.dmg

cd ..