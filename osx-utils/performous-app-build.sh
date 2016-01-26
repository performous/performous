set -o errexit
# the very first step is to check that dylibbundler exists,
# without it the bundle would be broken

function asksure {
echo -n "$1"
while read -r -n 1 -s answer; do
  if [[ $answer = [YyNn] ]]; then
    [[ $answer = [Yy] ]] && retval=0
    [[ $answer = [Nn] ]] && retval=1
    break
  fi
done

echo # just a final linefeed, optics...

return $retval
}

if which dylibbundler &> /dev/null; then
    echo "dylibbundler found!"
else
    echo "dylibbundler not found! you need to install it before creating the bundle."
    exit
fi

if which appdmg &> /dev/null; then
    echo "appdmg found!"
    FANCY_DMG=1
else
    echo "appdmg not found!"
if which npm &> /dev/null; then
    echo "npm found!"
if asksure "appdmg is not installed, would you like to install it? (y/n)"; then
 sudo npm install -g https://github.com/LinusU/node-appdmg.git
FANCY_DMG=1
else
echo "Will build DMG without the fancy style then..."
FANCY_DMG=0
fi
else
echo "npm not found!"
echo "Will build DMG without the fancy style then..."
FANCY_DMG=0
fi
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

cmake -DCMAKE_INSTALL_PREFIX=$TEMPDIR -DENABLE_TOOLS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=1 -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 -DFontconfig_INCLUDE_DIR=/opt/local/include/fontconfig -DPng_INCLUDE_DIR=/opt/local/include/libpng -DAVCodec_INCLUDE_DIR=/opt/local/include/libavcodec -DAVFormat_INCLUDE_DIR=/opt/local/include/libavformat -DSWScale_INCLUDE_DIR=/opt/local/include/libswscale -DFreetype_INCLUDE_DIR=/opt/local/include/freetype2/ -DLibXML2_LIBRARY=/opt/local/lib/libxml2.dylib -DLibXML2_INCLUDE_DIR=/opt/local/include/libxml2 -DLibXML++Config_INCLUDE_DIR=/opt/local/lib/libxml++-2.6/include -DGettext_LIBRARY=/opt/local/lib/libgettextlib.dylib  -DGettext_INCLUDE_DIR=/opt/local/include/ -DGlibmmConfig_INCLUDE_DIR=/opt/local/lib/glibmm-2.4/include -DGlibConfig_INCLUDE_DIR=/opt/local/lib/glib-2.0/include -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_C_FLAGS="-arch x86_64" -DSHARE_INSTALL=Resources -DLOCALE_DIR=Resources/Locales -DCMAKE_CXX_FLAGS="-Wno-unused-function -Wno-unused-local-typedefs -Wno-ignored-qualifiers -stdlib=libc++ -lc++ -lc++abi -arch x86_64 -L/opt/local/lib -lintl -Wl,-framework -Wl,CoreFoundation,-headerpad_max_install_names" -DCMAKE_EXE_LINKER_FLAGS="-ldl -stdlib=libc++ -lc++ -lc++abi -arch x86_64" -DCMAKE_MODULE_LINKER_FLAGS="-ldl -stdlib=libc++ -lc++ -lc++abi -arch x86_64" -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_SHARED_LINKER_FLAGS="-ldl -stdlib=libc++ -lc++ -lc++abi -arch x86_64" -DCMAKE_STATIC_LINKER_FLAGS="-ldl -stdlib=libc++ -lc++ -lc++abi -arch x86_64" ../..
make -j8 install # You can change the -j value in order to spawn more build threads.

# then create the rest of the app bundle

mv "$TEMPDIR/bin" "$BINDIR"

cp ../resources/performous-launcher "$BINDIR"
cp ../resources/performous.icns "$RESDIR"
cp ../resources/Info.plist "$TEMPDIR"

mkdir -p $FRAMEWORKDIR
mkdir -p $LIBDIR

dylibbundler -od -b -x "$BINDIR/performous" -d "$LIBDIR" -p @executable_path/../Resources/lib/
declare -a performous_tools
performous_tools=(gh_fsb_decrypt gh_xen_decrypt itg_pck ss_adpcm_decode ss_archive_extract ss_chc_decode ss_cover_conv ss_extract ss_ipu_conv ss_pak_extract)
for i in "${performous_tools[@]}"
do
if [ -f "$BINDIR/$i" ]; then dylibbundler -of -b -x "$BINDIR/$i" -d "$LIBDIR" -p @executable_path/../Resources/lib/; fi
done

# cp -av /opt/local/lib/pango $LIBDIR/ Pango modules are toast as of 1.38 and onwards.

OLDPREFIX=/opt/local
NEWPREFIX=$LIBDIR

mkdir -p $ETCDIR/fonts
cp -av /opt/local/etc/fonts $ETCDIR
# cp -av /opt/local/etc/pango $ETCDIR pangorc doesn't exist anymore either.
# cd $ETCDIR/pango

# sed -i '' -e "s|$OLDPREFIX\/etc\/pango\/|\.\.\/Resources\/etc\/pango\/|g" pangorc
# sed -i '' -e "s|$OLDPREFIX|\.\.\/\.\.\/\.\.\/\.\.|g" pango.modules

cd $ETCDIR/fonts
sed -i '' -e 's|\/opt\/local/share|\.\.\/\.\.\/\.\.\/Resources|g' fonts.conf
sed -i '' -e 's|\/opt\/local/var/cache|\~\/\.cache|g' fonts.conf
sed -i '' -e 's|\<\!-- Font directory list --\>|\<\!-- Font directory list --\>\
    <dir>\.\.\/\.\.\/pixmaps</dir>|g' fonts.conf

# cd $LIBDIR/pango/
# cd `find . -type d -maxdepth 1 -mindepth 1`
# cd `find . -type d -maxdepth 1 -mindepth 1` 

#for if in `pwd`/*.so
#do
#dylibbundler -x "$if" -p @executable_path/../Resources/lib/ -d "$LIBDIR"
#done 


cd "$CURRDIR"

# then build the disk image

if $FANCY_DMG == 0 then
ln -sf /Applications "${CURRDIR}/out/Applications"
rm "${CURRDIR}/out/.DS_Store"
/usr/bin/hdiutil create -ov -srcfolder out -volname Performous -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW RWPerformous.dmg
/usr/bin/hdiutil convert -ov RWPerformous.dmg -format UDZO -imagekey zlib-level=9 -o Performous.dmg
rm -f RWPerformous.dmg
cd ..
elif $FANCY_DMG == 1 then
rm "${CURRDIR}/Performous.dmg"
appdmg "${CURRDIR}/resources/dmg_spec.json" "${CURRDIR}/Performous.dmg"
fi