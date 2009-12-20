#!/bin/sh -ex
# Run this script under MSYS started from the VS2008 Command Prompt.
# Perl, Python, and CMake must be installed and on the PATH.
# Just a base installation of MSYS is needed - you don't need any
# additional packages or any part of MinGW.
#
# If you don't have wget on your PATH, place a copy of the source code
# for GNU Wget in the same folder as this script and it will do the rest ;)
# (Note that this script is coded to specific versions of the upstream
#  packages that I know work together - update them if you want, but the
#  wget (which currently *must* be wget-1.11.4.tar.bz2) invocation will
#  fetch said specific versions.)
#
# So grab yourself a wget.exe binary and stick it in your PATH, or just
# download this, put it in the same folder as this script, and let this
# script automatically build it for you:
#
#   http://ftp.gnu.org/pub/gnu/wget/wget-1.11.4.tar.bz2
#
# There must not be any spaces in the absolute path to the folder.

echo "Stump's Performous MSVC dependency setup script"
echo ''
echo 'If you have issues, be sure that you are running this script'
echo 'from an MSYS prompt that was started from a Visual Studio 2008'
echo 'Command Prompt (type something like "start C:\\msys\\1.0\\msys.bat"'
echo 'from such a prompt).  You must have Perl, Python, and CMake on your PATH.'
echo ''

# Allow the user to keep temporary files around.
# (Set the KEEPTEMP environment variable to any value.)
if test -n "$KEEPTEMP"; then
  RM_RF=true
else
  RM_RF="rm -rf"
fi

echo 'Checking the environment.'

if test x"$MACHTYPE" != xi686-pc-msys; then
  echo 'This script must be run under MSYS.' >&2
  exit 1
fi

# Check for spaces in the absolute path to us.
set foo `pwd`
if test "$#" -ne 2; then
  echo 'There cannot be spaces in the absolute path to the ' >&2
  echo 'folder from which this script is started.' >&2
  exit 1
fi

echo 'Looking for the commands we need...'
type basename
type bzip2 #--version >/dev/null 2>&1 && echo 'bzip2 is working...'
type cat
type cl #'-?' </dev/null >/dev/null 2>&1 && echo 'cl is working...'
type cmake #--version >/dev/null 2>&1 && echo 'cmake is working...'
alias cmake="cmake -G \"NMake Makefiles\""
type cmd #//c echo cmd is working...
type cp #--version >/dev/null 2>&1 && echo 'cp is working...'
type cscript #'-?' </dev/null >/dev/null 2>&1 && echo 'cscript is working...'
type dirname
type expr
type grep #--version >/dev/null 2>&1 && echo 'grep is working...'
type gzip #--version >/dev/null 2>&1 && echo 'gzip is working...'
type lib #'-?' </dev/null >/dev/null 2>&1 && echo 'lib is working...'
type link #'-?' </dev/null >/dev/null 2>&1 && echo 'link is working...'
type make #--version >/dev/null 2>&1 && echo 'make is working...'
type mkdir #--version >/dev/null 2>&1 && echo 'mkdir is working...'
type mt #'-?' </dev/null >/dev/null 2>&1 && echo 'mt is working...'
type mv #--version >/dev/null 2>&1 && echo 'mv is working...'
type nmake #'-?' </dev/null >/dev/null 2>&1 && echo 'nmake is working...'
type patch #--version >/dev/null 2>&1 && echo 'patch is working...'
type perl #-e '$_ = "perl is working...\n"; print'
type python #-c 'msg = "python is working..."; print msg'
type rc #'-?' </dev/null >/dev/null 2>&1 && echo 'rc is working...'
type regsvr32
type rm #--version >/dev/null 2>&1 && echo 'rm is working...'
type sed #--version >/dev/null 2>&1 && echo 'sed is working...'
type sh #--version >/dev/null 2>&1 && echo 'sh is working...'
type sleep #--version >/dev/null 2>&1 && echo 'sleep is working...'
type tar #--version >/dev/null 2>&1 && echo 'tar is working...'
type touch #--version >/dev/null 2>&1 && echo 'touch is working...'
type vcbuild #'-?' </dev/null >/dev/null 2>&1 && echo 'vcbuild is working...'
type which

# work around a bug in the VS2008 installer...
regsvr32 -s "`which VCProjectEngine.dll`"

# Embed manifests when the makefiles do it incorrectly.
# See: http://msdn.microsoft.com/en-us/library/ms235591.aspx
manifestify () {
  if ! test -f "$1".manifest; then
    # Link to MSVCR90 and run as an ordinary user.
    cat >"$1".manifest <<"EOF"
<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level='asInvoker' uiAccess='false' />
      </requestedPrivileges>
    </security>
  </trustInfo>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.VC90.CRT" version="9.0.21022.8" processorArchitecture="x86" publicKeyToken="1fc8b3b9a1e18e3b" />
    </dependentAssembly>
  </dependency>
</assembly>
EOF
  fi
  if test "`expr "$1" : '.*\.dll$'`" -ne 0; then
    mt -manifest "$1".manifest -outputresource:"$1"\;2
  else
    mt -manifest "$1".manifest -outputresource:"$1"\;1
  fi
}

mkdir -pv ../bin ../include ../lib
export PATH=`cd ../bin && pwd`:$PATH
export INCLUDE=`cd ../include && cmd //c echo $(pwd)`\;$INCLUDE
export LIB=`cd ../lib && cmd //c echo $(pwd)`\;$LIB

if ! type wget >/dev/null 2>&1; then
  if ! test -f wget-1.11.4.tar.bz2; then
    echo 'Please place a wget.exe binary on your PATH.  Or you can go the' >&2
    echo 'awesome boostrapping route and place a copy of wget-1.11.4.tar.bz2' >&2
    echo '(get it from your favorite GNU mirror) in this directory, run this' >&2
    echo 'script again, and watch the script build it for you ;)' >&2
    echo '' >&2
    echo '  http://ftp.gnu.org/pub/gnu/wget/wget-1.11.4.tar.bz2' >&2
    exit 1
  fi
  tar jxvf wget-1.11.4.tar.bz2
  cd wget-1.11.4
  cmd //c configure.bat --msvc
  nmake NO_SSL=1
  cp -v src/wget.exe ../../bin
  cd ..
  $RM_RF wget-1.11.4
fi

# The list of source archives we need.
cat >PACKAGES.wget <<"EOF"
ftp://ftp.info-zip.org/pub/infozip/src/unzip60.tgz
http://www.two-sdg.demon.co.uk/curbralan/code/dirent/dirent.h
http://www.two-sdg.demon.co.uk/curbralan/code/dirent/dirent.c
http://msinttypes.googlecode.com/files/msinttypes-r26.zip
http://www.zlib.net/zlib-1.2.3.tar.bz2
http://www.portaudio.com/archives/pa_stable_v19_20071207.tar.gz
http://www.libsdl.org/release/SDL-1.2.14.tar.gz
http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/proxy-libintl-dev_20090911_win32.zip
http://ftp.gnome.org/pub/GNOME/sources/glib/2.22/glib-2.22.3.tar.bz2
http://prdownloads.sourceforge.net/sevenzip/7z465.tar.bz2
http://prdownloads.sourceforge.net/boost/boost_1_41_0.7z
http://prdownloads.sourceforge.net/libpng/libpng-1.2.41.tar.gz
http://www.cairographics.org/releases/pixman-0.17.2.tar.gz
http://www.cairographics.org/releases/cairo-1.8.8.tar.gz
http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.11.1.tar.gz
http://kde-mirror.freenux.org/stable/4.3.4/win32/freetype-vc90-2.3.7-1-src.tar.bz2
#http://pkgconfig.freedesktop.org/releases/pkg-config-0.23.tar.gz
ftp://xmlsoft.org/libxml2/libxml2-2.7.6.tar.gz
http://kde-mirror.freenux.org/stable/4.3.4/win32/fontconfig-vc90-2.4.2-3-src.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/pango/1.26/pango-1.26.1.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/atk/1.28/atk-1.28.0.tar.bz2
http://www.ijg.org/files/jpegsrc.v7.tar.gz
ftp://ftp.remotesensing.org/pub/libtiff/tiff-3.9.2.tar.gz
http://ftp.gnome.org/pub/GNOME/sources/gtk+/2.18/gtk+-2.18.5.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/librsvg/2.26/librsvg-2.26.0.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/libsigc++/2.2/libsigc++-2.2.4.2.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/glibmm/2.22/glibmm-2.22.1.tar.bz2
http://ftp.gnome.org/pub/GNOME/sources/libxml++/2.26/libxml++-2.26.1.tar.bz2
http://prdownloads.sourceforge.net/glew/glew-1.5.1-src.tgz
#ftp://ftp.imagemagick.org/pub/ImageMagick/windows/ImageMagick-6.5.8-6.7z
http://ffmpeg.arrozcru.org/autobuilds/ffmpeg/dev/shared/ffmpeg-r20817-swscale-r29978-mingw32-shared-dev.7z
http://ffmpeg.arrozcru.org/autobuilds/ffmpeg/mingw32/shared/ffmpeg-r20817-swscale-r29978-mingw32-shared.7z
http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.10.tar.gz
EOF

if test "$0" -nt download-stamp; then
  ../wget -c -i PACKAGES.wget
  sleep 2
  touch download-stamp
fi

$RM_RF PACKAGES.wget

# Placeholder vcproj for when slns refer to missing ones, like glib's does.
cat >dummy.vcproj <<"EOF"
<?xml version="1.0" encoding="iso-8859-1"?>
<VisualStudioProject Version="9.00">
 <Platforms><Platform Name="Win32" /></Platforms>
 <Configurations>
  <Configuration Name="@CONFIG@" ConfigurationType="10" />
 </Configurations>
</VisualStudioProject>
EOF

# Dummy implementation of the cygpath command that the autotools seem to like.
cat >dummy-cygpath <<"EOF"
#!/bin/sh -e
for arg; do
  case "$arg" in
    -*)
      ;;
    *)
      echo "$arg"
      exit 0
      ;;
  esac
done
exit 1
EOF

if ! test -f ../bin/unzip.exe; then
  tar zxvf unzip60.tgz
  cd unzip60
  nmake -f win32/Makefile unzip.exe
  cp -v unzip.exe ../../bin
  cd ..
  $RM_RF unzip60
fi

if ! test -f ../lib/dirent.lib; then
  cp -v dirent.h ../include
  cl -MD -Fodirent.obj -c dirent.c
  lib -verbose -out:dirent.lib dirent.obj
  cl -MT -Fodirentmt.obj -c dirent.c
  lib -verbose -out:direntmt.lib direntmt.obj
  cp -v dirent.lib direntmt.lib ../lib
  $RM_RF dirent.lib dirent.obj direntmt.lib direntmt.obj
fi

if ! test -f ../include/stdint.h; then
  mkdir -pv msinttypes
  cd msinttypes
  unzip -o ../msinttypes-r26.zip
  cp -v inttypes.h stdint.h ../../include
  cd ..
  $RM_RF msinttypes
fi

if ! test -f ../lib/z.lib; then
  tar jxvf zlib-1.2.3.tar.bz2
  cd zlib-1.2.3
  sed -e 's/1,2,2,0/1,2,3,0/g' win32/zlib1.rc >win32/zlib1.rc.new
  mv -v win32/zlib1.rc.new win32/zlib1.rc
  nmake -f win32/Makefile.msc
  cp -v zlib.h zconf.h ../../include
  cp -v zlib1.dll ../../bin
  cp -v zlib.lib zdll.lib ../../lib
  cp -v ../../lib/zdll.lib ../../lib/z.lib
  cd ..
  $RM_RF zlib-1.2.3
fi

if ! test -f ../lib/portaudio.lib; then
  tar zxvf pa_stable_v19_20071207.tar.gz
  cd portaudio/build/msvc
  # This python and sed remove all references to ASIO, which we don't feel
  # like obtaining so we can build with it.
  cat >fixasio.py <<"EOF"
import xml.dom.minidom
import sys
vcproj = xml.dom.minidom.parse(sys.argv[1])
for f in vcproj.getElementsByTagName('File'):
  if f.attributes['RelativePath'].nodeValue.startswith(r'..\..\src\hostapi\asio'):
    f.parentNode.removeChild(f)
for t in vcproj.getElementsByTagName('Tool'):
  if t.attributes['Name'].nodeValue == 'VCCLCompilerTool':
    t.attributes['PreprocessorDefinitions'].nodeValue += ';PA_NO_ASIO'
vcproj.writexml(sys.stdout)
EOF
  python fixasio.py portaudio.vcproj >portaudio.vcproj.new
  mv -v portaudio.vcproj.new portaudio.vcproj
  $RM_RF fixasio.py
  sed -e '/^PaAsio/s/.*/;&/' portaudio.def >portaudio.def.new
  mv -v portaudio.def.new portaudio.def
  vcbuild -useenv portaudio.sln 'Release|Win32'
  cp -v ../../include/portaudio.h ../../../../include
  cp -v Win32/Release/portaudio_x86.dll ../../../../bin
  cp -v Win32/Release/portaudio_x86.lib ../../../../lib/portaudio.lib
  cd ../../..
  $RM_RF portaudio
fi

if ! test -f ../lib/SDL.lib; then
  tar zxvf SDL-1.2.14.tar.gz
  cd SDL-1.2.14
  unzip -o VisualC.zip
  cd VisualC
  # Upstream forgot this?
  cat >fixversion.sed <<"EOF"
/<\/Files>/i\
<File RelativePath=".\\Version.rc" />
EOF
  sed -f fixversion.sed SDL/SDL.vcproj >SDL/SDL.vcproj.new
  mv -v SDL/SDL.vcproj.new SDL/SDL.vcproj
  $RM_RF fixversion.sed
  vcbuild -useenv SDL.sln 'Release|Win32'
  mkdir -pv ../../../include/SDL
  cp -av ../include/*.h ../../../include/SDL
  cp -v SDL/Release/SDL.dll ../../../bin
  cp -v SDLmain/Release/SDLmain.lib SDL/Release/SDL.lib ../../../lib
  cd ../..
  $RM_RF SDL-1.2.14
fi

if ! test -f ../lib/libintl.lib; then
  mkdir -pv proxy-libintl-dev
  cd proxy-libintl-dev
  unzip -o ../proxy-libintl-dev_20090911_win32.zip
  cp -v include/libintl.h ../../include
  cd src/proxy-libintl
  cl -MD -Folibintl.obj -c libintl.c
  lib -verbose -out:intl.lib libintl.obj
  cl -MT -Folibintlmt.obj -c libintl.c
  lib -verbose -out:intlmt.lib libintlmt.obj
  cp -v intl.lib intlmt.lib ../../../../lib
  cp -v ../../../../lib/intl.lib ../../../../lib/libintl.lib
  cd ../../..
  $RM_RF proxy-libintl-dev
fi

if ! test -f ../lib/glib-2.0.lib; then
  tar jxvf glib-2.22.3.tar.bz2
  cd glib-2.22.3/build/win32/vs9
  # Upstream forgot these?
  cat >fixgio.sed <<"EOF"
/AdditionalDependencies/s/="/&Dnsapi.lib /
/<\/Files>/i\
<File RelativePath="..\\..\\..\\gio\\gio.rc" />
EOF
  sed -f fixgio.sed gio.vcproj >gio.vcproj.new
  mv -v gio.vcproj.new gio.vcproj
  $RM_RF fixgio.sed
  sed -e '/FileDescription/s/GLib/GIO/' -e 's/libglib/libgio/g' ../../../glib/glib.rc >../../../gio/gio.rc
  # Use our dummy vcproj since this one is missing.
  sed -e 's/@CONFIG@/Release|Win32/' ../../../../dummy.vcproj >testglib.vcproj
  vcbuild -useenv glib.sln 'Release|Win32'
  cp -v Release/*.exe ../../../../dependencies/Win32/vs9/bin
  cd ../../../..
  # librsvg needs these
  for file in gasyncinitable.h ginitable.h ginetaddress.h ginetsocketaddress.h \
    gsocketaddress.h giostream.h gfileiostream.h gnetworkaddress.h \
    gnetworkservice.h gresolver.h gsocket.h gsocketaddressenumerator.h \
    gsocketclient.h gsocketconnectable.h gsocketconnection.h gsocketlistener.h \
    gsocketcontrolmessage.h gsocketservice.h gtcpconnection.h gsrvtarget.h \
    gthreadedsocketservice.h; do
    cp -v glib-2.22.3/gio/"$file" dependencies/Win32/vs9/include/glib-2.0/gio
  done
  $RM_RF glib-2.22.3
  mv -v dependencies/Win32/vs9/bin/* ../bin
  if test -f ../bin/gspawn-win64-helper.exe; then
    mv -v ../bin/gspawn-win64-helper.exe ../bin/gspawn-win32-helper.exe
  fi
  if test -d ../include/glib-2.0; then
    rm -rf ../include/glib-2.0
  fi
  mv -v dependencies/Win32/vs9/include/* ../include
  if test -d ../lib/glib-2.0; then
    rm -rf ../lib/glib-2.0
  fi
  mv -v dependencies/Win32/vs9/lib/* ../lib
  $RM_RF dependencies
fi

if ! test -f ../bin/7z.exe; then
  mkdir -pv 7z465
  tar -C 7z465 -jxvf 7z465.tar.bz2
  cd 7z465/CPP/7zip/UI/Console
  nmake
  manifestify O/7z.exe
  cp -v O/7z.exe ../../../../../../bin
  cd ../../../../..
  $RM_RF 7z465
fi

if ! test -f ../lib/boost_filesystem-vc90-mt-1_41.lib; then
  7z x -y boost_1_41_0.7z
  cd boost_1_41_0
  for file in \
    boost/program_options/detail/value_semantic.hpp \
    boost/program_options/options_description.hpp \
    libs/program_options/src/options_description.cpp \
    libs/program_options/src/value_semantic.cpp; do
      tr -d '\r' <"$file" >"$file".new
      mv -v "$file".new "$file"
  done
  patch -Np0 <<"EOF"
--- boost/program_options/detail/value_semantic.hpp.old	2009-12-14 02:32:44 -0500
+++ boost/program_options/detail/value_semantic.hpp	2009-12-14 02:35:08 -0500
@@ -10,12 +10,11 @@
 
 namespace boost { namespace program_options { 
 
-    extern BOOST_PROGRAM_OPTIONS_DECL std::string arg;
-    
     template<class T, class charT>
     std::string
     typed_value<T, charT>::name() const
     {
+        static std::string arg("arg");
         if (!m_implicit_value.empty() && !m_implicit_value_as_text.empty()) {
             std::string msg = "[=arg(=" + m_implicit_value_as_text + ")]";
             if (!m_default_value.empty() && !m_default_value_as_text.empty())
--- boost/program_options/options_description.hpp.old	2009-12-14 02:35:47 -0500
+++ boost/program_options/options_description.hpp	2009-12-14 02:52:54 -0500
@@ -30,6 +30,8 @@
 /** Namespace for the library. */
 namespace program_options {
 
+    BOOST_STATIC_CONSTANT(unsigned, m_default_line_length, 80);
+        
     /** Describes one possible command line/config file option. There are two
         kinds of properties of an option. First describe it syntactically and
         are used only to validate input. Second affect interpretation of the
@@ -155,8 +157,6 @@
     */
     class BOOST_PROGRAM_OPTIONS_DECL options_description {
     public:
-        static const unsigned m_default_line_length;
-        
         /** Creates the instance. */
         options_description(unsigned line_length = m_default_line_length);
         /** Creates the instance. The 'caption' parameter gives the name of
--- libs/program_options/src/options_description.cpp.old	2009-12-14 02:39:02 -0500
+++ libs/program_options/src/options_description.cpp	2009-12-14 02:40:06 -0500
@@ -201,8 +201,6 @@
         return *this;
     }
 
-    const unsigned options_description::m_default_line_length = 80;
-
     options_description::options_description(unsigned line_length)
     : m_line_length(line_length)
     {}
--- libs/program_options/src/value_semantic.cpp.old	2009-12-14 02:40:19 -0500
+++ libs/program_options/src/value_semantic.cpp	2009-12-14 02:40:57 -0500
@@ -64,12 +64,10 @@
     }
 #endif
 
-    BOOST_PROGRAM_OPTIONS_DECL std::string arg("arg");
-
     std::string
     untyped_value::name() const
     {
-        return arg;
+        return "arg";
     }
     
     unsigned 
EOF
  if ! test -f ../../bin/bjam.exe; then
    cd tools/jam/src
    cmd //c build.bat
    cp -v bin.ntx86/bjam.exe ../../../../../bin
    cd ../../..
  fi
  bjam stage toolset=msvc variant=release link=shared threading=multi runtime-link=shared
  if test -d ../../include/boost; then
    rm -rf ../../include/boost
  fi
  mv -v boost ../../include
  cp -v stage/lib/*.dll ../../bin
  for file in stage/lib/boost*.lib; do
    if test "`expr "$file" : '.*-mt\.lib$'`" -eq 0; then
      cp -v "$file" ../../lib/"`basename "$file"`"
      cp -v "$file" ../../lib/lib"`basename "$file"`"
    fi
  done
  cd ..
  $RM_RF boost_1_41_0
fi

if ! test -f ../lib/png12.lib; then
  tar zxvf libpng-1.2.41.tar.gz
  cd libpng-1.2.41/scripts
  # The former never actually gets defined by the code, and
  # the latter is needed later on but is commented out.
  sed -e '/png_set_strip_error_numbers/s/.*/;&/' -e '/; png_get_libpng_ver/s/; //' pngw32.def >pngw32.def.new
  mv -v pngw32.def.new pngw32.def
  cd ../projects/visualc71
  vcbuild -upgrade libpng.vcproj
  # Use the copy of zlib that we just built, not libpng's own copy.
  sed -e '/Name="VCLinkerTool"/s/.*/& AdditionalDependencies="zdll.lib"/' libpng.vcproj >libpng.vcproj.new
  mv -v libpng.vcproj.new libpng.vcproj
  vcbuild -useenv libpng.vcproj 'DLL Release|Win32'
  cp -v Win32_DLL_Release/libpng13.dll ../../../../bin
  cp -v ../../png.h ../../pngconf.h ../../../../include
  cp -v Win32_DLL_Release/libpng13.lib ../../../../lib/libpng.lib
  cp -v ../../../../lib/libpng.lib ../../../../lib/png12.lib
  cd ../../..
  $RM_RF libpng-1.2.41
fi

if ! test -f ../lib/pixman-1.lib; then
  tar zxvf pixman-0.17.2.tar.gz
  cd pixman-0.17.2/pixman
  make -f Makefile.win32 CFG=release SSE2=off
  cp -v pixman.h pixman-version.h ../../../include
  cp -v release/pixman-1.lib ../../../lib
  cd ../..
  $RM_RF pixman-0.17.2
fi

if ! test -f ../lib/cairo.lib; then
  tar zxvf cairo-1.8.8.tar.gz
  cd cairo-1.8.8
  make -f Makefile.win32 CFG=release PIXMAN_LIBS=pixman-1.lib
  cp -v cairo-version.h src/cairo-features.h src/cairo.h src/cairo-deprecated.h src/cairo-win32.h src/cairo-ps.h src/cairo-pdf.h src/cairo-svg.h ../../include
  cp -v src/release/cairo.dll ../../bin
  cp -v src/release/cairo.lib ../../lib
  cd ..
  $RM_RF cairo-1.8.8
fi

if ! test -f ../lib/iconv.lib; then
  tar zxvf libiconv-1.11.1.tar.gz
  cd libiconv-1.11.1
  # TODO: Get it to link correctly with proxy-libintl.  TODO: version info, latest version
  nmake -f Makefile.msvc PREFIX=`(cd ../.. && pwd)` NO_NLS=1 DLL=1
  cp -v include/iconv.h ../../include
  manifestify src/iconv.exe
  cp -v src/iconv.exe ../../bin
  cp -v lib/iconv.dll ../../bin
  cp -v lib/iconv.lib ../../lib
  cd ..
  $RM_RF libiconv-1.11.1
fi

if ! test -f ../lib/freetype.lib; then
#  tar jxvf freetype-2.3.11.tar.bz2
##  cp -v dummy-cygpath ../bin/cygpath
#  cd freetype-2.3.11
##  ./configure CC='cl -nologo' LD='link -nologo' --prefix=/ --disable-static
##  make
##  make DESTDIR=`pwd`/_inst install
#  cd builds/win32/vc2008
#  vcbuild -useenv freetype.sln 'LIB Release|Win32'
#  cd ../../..
#  cp -av include/ft2build.h include/freetype ../../include
#  rm -rf ../../include/freetype/internal
#  cp -v objs/win32/vc2008/freetype2311.lib ../../lib/freetype.lib
##  cp -v _inst/lib/libfreetype-6.dll ../../bin
##  cp -v _inst/lib/libfreetype.lib ../../lib/freetype6.lib
#  cd ..
##  rm -vf ../bin/cygpath
#  $RM_RF freetype-2.3.11
#  cat >../bin/freetype-config <<"EOF"
##!/bin/sh -e
#if test x"$1" == x"--libs"; then echo freetype.lib; fi
#exit 0
#EOF
  tar --strip-components=1 -jxvf freetype-vc90-2.3.7-1-src.tar.bz2
  cd freetype-vc90-2.3.7-1
  cmake -DCMAKE_BUILD_TYPE=Release .
  nmake
  cp -av include/ft2build.h include/freetype ../../include
  rm -rf ../../include/freetype/internal
  cp -v freetype.dll ../../bin
  cp -v freetype.lib ../../lib
  cd ..
  $RM_RF freetype-vc90-2.3.7-1
fi

#if ! test -f ../bin/pkg-config.exe; then
#  tar zxvf pkg-config-0.23.tar.gz
#  cd pkg-config-0.23
#  ./configure CC='cl -nologo -FI malloc.h' LD='link -nologo' LIBS='glib-2.0.lib dirent.lib advapi32.lib' --prefix=`(cd ../.. && pwd)`
#  make
#  cp -v pkg-config.exe ../../bin
#  cd ..
#  $RM_RF pkg-config-0.23
#fi

if ! test -f ../lib/xml2.lib; then
  tar zxvf libxml2-2.7.6.tar.gz
  cd libxml2-2.7.6/win32
  cscript configure.js compiler=msvc vcmanifest=yes zlib=yes sodir='$(PREFIX)\bin'
  nmake -f Makefile.msvc
  nmake -f Makefile.msvc install
  cp -av include/* ../../../include
  cp -v ../include/win32config.h ../../../include
  cp -v bin/libxml2.dll bin/xmllint.exe bin/xmlcatalog.exe ../../../bin
  cp -v lib/libxml2.lib ../../../lib/xml2.lib
  cd ../..
  $RM_RF libxml2-2.7.6
fi

if ! test -f ../lib/fontconfig.lib; then
  tar --strip-components=1 -jxvf fontconfig-vc90-2.4.2-3-src.tar.bz2
  cd fontconfig-vc90-2.4.2-3
  # End line problem, can't include in this file (strange problem, but work with an extern file), so fix with extern patch file
  patch -Np0 < ../fontconfig-cmake.patch
  # We're providing these ourselves...
  cat >fixcml.sed <<"EOF"
s/dirent.c//g
s/unistd.c//g
s/mmap.c//g
/^install(/i\
target_link_libraries( fontconfig dirent.lib )
EOF
  sed -f fixcml.sed src/CMakeLists.txt >src/CMakeLists.txt.new
  mv -v src/CMakeLists.txt.new src/CMakeLists.txt
  $RM_RF fixcml.sed
  cat >fixcmlsubdir.sed <<"EOF"
s%../src/dirent.c%%g
s%../src/unistd.c%%g
/^install(/i\
target_link_libraries( ${sub_dir_name} dirent.lib )
EOF
  for dir in fc-cache fc-cat fc-list fc-match; do
    sed -f fixcmlsubdir.sed "$dir"/CMakeLists.txt >"$dir"/CMakeLists.txt.new
    mv -v "$dir"/CMakeLists.txt.new "$dir"/CMakeLists.txt
  done
  $RM_RF fixcmlsubdir.sed
  cmake -DCMAKE_INSTALL_PREFIX=`(cd ../.. && pwd)` -DCMAKE_BUILD_TYPE=Release .
#  tar zxvf fontconfig-2.8.0.tar.gz
#  cp -v dummy-cygpath ../bin/cygpath
#  cd fontconfig-2.8.0
#  ./configure CC='cl -nologo -MD' LD='link -nologo' LIBXML2_CFLAGS=' ' LIBXML2_LIBS='xml2.lib' LIBS='iconv.lib zdll.lib dirent.lib' --prefix=/ --disable-shared
  # Properly #ifdef all unistd.h inclusions.
  cat >fixunistd.sed <<"EOF"
/#include *<unistd.h>/ {i\
#ifdef HAVE_UNISTD_H
a\
#endif
}
EOF
  for file in src/fcint.h src/fcatomic.c fc-cache/fc-cache.c fc-cat/fc-cat.c fc-list/fc-list.c fc-match/fc-match.c; do
    sed -f fixunistd.sed "$file" >"$file".new
    mv -v "$file".new "$file"
  done
  $RM_RF fixunistd.sed
  # Windows' access() is a bit different and doesn't define *_OK...
  for file in src/fccache.c fc-cache/fc-cache.c; do
    cat - "$file" >"$file".new <<"EOF"
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 0
EOF
    mv -v "$file".new "$file"
  done
  # Nor does Windows define this...
  for file in src/fcdir.c fc-cache/fc-cache.c; do
    cat - "$file" >"$file".new <<"EOF"
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
EOF
    mv -v "$file".new "$file"
  done
#  make V=1
#  cd ..
#  rm -vf ../bin/cygpath
#  $RM_RF fontconfig-2.8.0
  nmake
  nmake install
  cd ..
  $RM_RF fontconfig-vc90-2.4.2-3
fi

setup_glib () {
  if ! test -f glib/setup-for-msvc; then
    mkdir -p glib
    cd glib
    tar --strip-components=1 -jxvf ../glib-2.22.3.tar.bz2
    sed -e 's/@GLIB_VERSION@/2.22.3/g' gobject/glib-mkenums.in >gobject/glib-mkenums
    cp -v glibconfig.h.win32 glibconfig.h
    cp -v config.h.win32 config.h
    cat >fixmakemsc.sed <<"EOF"
/^ATK_LIBS/s/\$(ATK)\\[^\\]*\\//g
/^CAIRO_CFLAGS/s/=.*/=/
/^CAIRO_LIBS/s/=.*/= cairo.lib/
/^FONTCONFIG_LIBS/s/=.*\\/= /
/^FREETYPE2_LIBS/s/=.*/= freetype.lib/
/^GLIB_LIBS/s/\$(GLIB)\\[^\\]*\\//g
/^JPEG_LIBS/s/=.*/= jpeg.lib/
/^INTL_LIBS/s/=.*/= intl.lib/
/^LIBICONV_LIBS/s/\$(LIBICONV)\\[^\\]*\\//g
/^PANGO[A-Z0-9]*_LIBS/s/\$(PANGO)\\pango\\//g
/^PNG_LIBS/s/=.*/= libpng.lib/
/^TIFF_LIBS/s/=.*/= tiff.lib/
/^ZLIB_LIBS/s/=.*/= zdll.lib/
EOF
    sed -f fixmakemsc.sed build/win32/make.msc >build/win32/make.msc.new
    mv -v build/win32/make.msc.new build/win32/make.msc
    $RM_RF fixmakemsc.sed
    touch setup-for-msvc
    cd ..
  fi
}

if ! test -f ../lib/pango-1.0.lib; then
  setup_glib
  tar jxvf pango-1.26.1.tar.bz2
  cd pango-1.26.1/pango
  # Unmaintained makefiles ftw...
  cp -v module-defs-lang.c module-defs-lang.c.win32
  cat >fixmakefile.sed <<"EOF"
s/mapping.obj//g
s/-DSYSCONFDIR/-DLIBDIR=\\"\/lib\\" &/g
s/.exe : $(PKG_LINK)/.exe : /
/^pango_headers=/a\
pango-bidi-type.h \\
/pango-enum-types.obj/a\
pango-bidi-type.obj \\\
pango.res \\
/pangowin32.obj/a\
pangowin32.res \\
/pangocairo-context.obj/a\
pangocairo.res \\
/pangoft2.obj/a\
pangoft2.res \\
EOF
  sed -f fixmakefile.sed makefile.msc >makefile.msc.new
  mv -v makefile.msc.new makefile.msc
  $RM_RF fixmakefile.sed
  # Why doesn't this one have version info?
  sed -e 's/Pango/&Cairo/g' -e 's/pango/&cairo/g' pango.rc >pangocairo.rc
  # KDE-win32's fontconfig is too old to define extrablack.
  patch -Np0 <<"EOF"
--- pangofc-fontmap.c.old       2009-12-10 00:55:43 -0500
+++ pangofc-fontmap.c   2009-12-10 00:56:11 -0500
@@ -1360,10 +1360,8 @@
     return FC_WEIGHT_BOLD;
   else if (pango_weight <= (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2)
     return FC_WEIGHT_ULTRABOLD;
-  else if (pango_weight <= (PANGO_WEIGHT_HEAVY + PANGO_WEIGHT_ULTRAHEAVY) / 2)
-    return FC_WEIGHT_BLACK;
   else
-    return FC_WEIGHT_EXTRABLACK;
+    return FC_WEIGHT_BLACK;
 }
 
 static int
@@ -2045,10 +2043,8 @@
     return PANGO_WEIGHT_BOLD;
   else if (fc_weight <= (FC_WEIGHT_EXTRABOLD + FC_WEIGHT_BLACK) / 2)
     return PANGO_WEIGHT_ULTRABOLD;
-  else if (fc_weight <= (FC_WEIGHT_BLACK + FC_WEIGHT_EXTRABLACK) / 2)
-    return PANGO_WEIGHT_HEAVY;
   else
-    return PANGO_WEIGHT_ULTRAHEAVY;
+    return PANGO_WEIGHT_HEAVY;
 }
 
 static PangoStyle
EOF
  nmake -f makefile.msc
  mkdir -pv ../../../include/pango
  cp -v pango-attributes.h pango-bidi-type.h pango-break.h pangocairo.h pango-context.h pango-coverage.h pango-engine.h pango-enum-types.h pangofc-decoder.h pangofc-font.h pangofc-fontmap.h pango-features.h pango-font.h pango-fontmap.h pango-fontset.h pangoft2.h pango-glyph.h pango-glyph-item.h pango-gravity.h pango.h pango-item.h pango-language.h pango-layout.h pango-matrix.h pango-modules.h pango-ot.h pango-renderer.h pango-script.h pango-tabs.h pango-types.h pango-utils.h pangowin32.h pangoxft.h pangoxft-render.h pangox.h ../../../include/pango
  cp -v libpango*.dll ../../../bin  # FIXME: pangoft is broken because opentype/ won't compile under MSVC
  manifestify querymodules.exe
  manifestify testfonts.exe
  cp -v querymodules.exe ../../../bin/pango-querymodules.exe
  cp -v testfonts.exe ../../../bin/pango-testfonts.exe
  cp -v pango*-1.0.lib ../../../lib
  cd ../..
  $RM_RF pango-1.26.1
fi

if ! test -f ../lib/atk-1.0.lib; then
  setup_glib
  tar jxvf atk-1.28.0.tar.bz2
  cd atk-1.28.0/atk

# Since I don't feel like sitting through ./configure dozens of times and
# praying that it will eventually work, I just ripped apart Makefile.am...
# --- cut here ---
#!/bin/sh -e

ATK_SOURCES=`echo "atkaction.c atkcomponent.c atkdocument.c
  atkeditabletext.c atkgobjectaccessible.c atkhyperlink.c
  atkhyperlinkimpl.c atkhypertext.c atkimage.c atknoopobject.c
  atknoopobjectfactory.c atkobject.c atkobjectfactory.c
  atkregistry.c atkrelation.c atkrelationset.c atkselection.c
  atkstate.c atkstateset.c atkstreamablecontent.c atktable.c
  atktext.c atkutil.c atkmisc.c atkvalue.c atk-enum-types.c"`
ATK_HEADERS=`echo "atk.h atkaction.h atkcomponent.h
  atkdocument.h atkeditabletext.h atkgobjectaccessible.h
  atkhyperlink.h atkhyperlinkimpl.h atkhypertext.h
  atknoopobject.h atknoopobjectfactory.h atkobject.h
  atkobjectfactory.h atkimage.h atkregistry.h atkrelation.h
  atkrelationtype.h atkrelationset.h atkselection.h atkstate.h
  atkstateset.h atkstreamablecontent.h atktable.h atktext.h
  atkutil.h atkmisc.h atkvalue.h"`

glib-genmarshal --prefix=atk_marshal atkmarshal.list --header >atkmarshal.h
glib-genmarshal --prefix=atk_marshal atkmarshal.list --body >atkmarshal.c

perl ../../glib/gobject/glib-mkenums \
  --fhead "#if defined(ATK_DISABLE_SINGLE_INCLUDES) && !defined (__ATK_H_INSIDE__) && !defined (ATK_COMPILATION)\n#error \"Only <atk/atk.h> can be included directly.\"\n#endif\n\n#ifndef __ATK_ENUM_TYPES_H__\n#define __ATK_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
  --fprod " /* enumerations from \"@filename@\" */\n" \
  --vhead "GType @enum_name@_get_type (void);\n#define ATK_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
  --ftail "G_END_DECLS\n\n#endif /* __ATK_ENUM_TYPES_H__ */" \
    $ATK_HEADERS >atk-enum-types.h

perl ../../glib/gobject/glib-mkenums \
  --fhead "#include <atk.h>" \
  --fprod "\n /* enumerations from \"@filename@\" */" \
  --vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
  --vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
  --vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
    $ATK_HEADERS >atk-enum-types.c

sed -e 's/^#undef VERSION$/#define VERSION "1.28.0"/' ../config.h.in >../config.h
sed -e 's/@ATK_MAJOR_VERSION@/1/g' \
    -e 's/@ATK_MINOR_VERSION@/28/g' \
    -e 's/@ATK_MICRO_VERSION@/0/g' \
    -e 's/@ATK_VERSION@/1.28.0/g' \
    -e 's/@LT_CURRENT_MINUS_AGE@/0/g' atk.rc.in >atk.rc
rc atk.rc
(echo EXPORTS; cl -EP -DINCLUDE_VARIABLES -DG_OS_WIN32 -DALL_FILES atk.symbols | sed -e '/^$$/d' -e 's/^/ /' -e 's/G_GNUC_[^ ]*//g') >atk.def
cl -I . -I .. -I ../../../include/glib-2.0 -I ../../../lib/glib-2.0/include \
  -DG_DISABLE_DEPRECATED -DATK_DISABLE_DEPRECATED -DATK_COMPILATION -DATK_LOCALEDIR="\"/build/share/locale\"" \
  -Felibatk-1.0-0.dll $ATK_SOURCES atk.res -link -dll -def:atk.def -implib:atk-1.0.lib \
  glib-2.0.lib gobject-2.0.lib
cp -v libatk-1.0-0.dll ../../../bin
mkdir -pv ../../../include/atk
cp -v $ATK_HEADERS atk-enum-types.h ../../../include/atk
cp -v atk-1.0.lib ../../../lib
# --- cut here ---
# (Note: I would have used cmake, but I didn't feel like trying to get it to
#  cooperate with glib-genmarshal and glib-mkenums.  Trust me, I tried.)

  cd ../..
  $RM_RF atk-1.28.0
fi

if ! test -f ../lib/jpeg.lib; then
  tar zxvf jpegsrc.v7.tar.gz
  cd jpeg-7
  # The build systems that come with libjpeg vehemently refuse to allow it to
  # be compiled as a DLL, so we do so ourselves with cmake.
  echo EXPORTS >libjpeg.def
  echo jpeg_std_error >>libjpeg.def
  grep -h '^EXTERN([a-zA-Z0-9_ *]*) j[a-zA-Z0-9_]*' jpeglib.h jpegint.h | sed -e 's/EXTERN([a-zA-Z0-9_ *]*) \(j[a-zA-Z0-9_]*\).*/\1/' >>libjpeg.def
  cp -v jconfig.vc jconfig.h
  cat >CMakeLists.txt <<"EOF"
add_library(libjpeg SHARED jcapimin.c jcapistd.c jcarith.c jctrans.c
        jcparam.c jdatadst.c jcinit.c jcmaster.c jcmarker.c jcmainct.c
        jcprepct.c jccoefct.c jccolor.c jcsample.c jchuff.c jcdctmgr.c
        jfdctfst.c jfdctflt.c jfdctint.c jdapimin.c jdapistd.c jdarith.c
        jdtrans.c jdatasrc.c jdmaster.c jdinput.c jdmarker.c jdhuff.c
        jdmainct.c jdcoefct.c jdpostct.c jddctmgr.c jidctfst.c jidctflt.c
        jidctint.c jdsample.c jdcolor.c jquant1.c jquant2.c jdmerge.c
        jaricom.c jcomapi.c jutils.c jerror.c jmemmgr.c jmemnobs.c libjpeg.def)
add_executable(cjpeg cjpeg.c rdppm.c rdgif.c rdtarga.c rdrle.c rdbmp.c
        rdswitch.c cdjpeg.c)
add_executable(djpeg djpeg.c wrppm.c wrgif.c wrtarga.c wrrle.c wrbmp.c
        rdcolmap.c cdjpeg.c)
add_executable(jpegtran jpegtran.c rdswitch.c cdjpeg.c transupp.c)
add_executable(rdjpgcom rdjpgcom.c)
add_executable(wrjpgcom wrjpgcom.c)
target_link_libraries(cjpeg libjpeg)
target_link_libraries(djpeg libjpeg)
target_link_libraries(jpegtran libjpeg)
target_link_libraries(rdjpgcom libjpeg)
target_link_libraries(wrjpgcom libjpeg)
EOF
  cmake -DCMAKE_BUILD_TYPE=Release .
  nmake
  cp -v jconfig.h jerror.h jmorecfg.h jpeglib.h ../../include
  cp -v libjpeg.dll *.exe ../../bin
  cp -v libjpeg.lib ../../lib/jpeg.lib
  cd ..
  $RM_RF jpeg-7
fi

if ! test -f ../lib/tiff.lib; then
  tar zxvf tiff-3.9.2.tar.gz
  cd tiff-3.9.2
  cat >fixnmakeopt.sed <<"EOF"
s/^#ZIP/ZIP/
s/^#ZLIB/ZLIB/
/^ZLIBDIR/d
/^ZLIB_INCLUDE/s/=.*/=/
/^ZLIB_LIB/s/=.*/= zdll.lib/
s/^#JPEG/JPEG/
/^JPEGDIR/d
/^JPEG_INCLUDE/s/=.*/=/
/^JPEG_LIB/s/=.*/= jpeg.lib/
EOF
  sed -f fixnmakeopt.sed nmake.opt >nmake.opt.new
  mv -v nmake.opt.new nmake.opt
  $RM_RF fixnmakeopt.sed
  sed -e 's/libtiff\.lib/libtiff_i.lib/g' tools/Makefile.vc >tools/Makefile.vc.new
  mv -v tools/Makefile.vc.new tools/Makefile.vc
  echo _TIFFCheckMalloc >>libtiff/libtiff.def
  nmake -f Makefile.vc
  cp -v libtiff/tiff.h libtiff/tiffio.h libtiff/tiffvers.h libtiff/tiffconf.h ../../include
  for file in tools/*.exe; do
    manifestify "$file"
  done
  cp -v libtiff/libtiff.dll tools/*.exe ../../bin
  cp -v libtiff/libtiff_i.lib ../../lib/tiff.lib
  cd ..
  $RM_RF tiff-3.9.2
fi

if ! test -f ../lib/gtk-win32-2.0.lib; then
  setup_glib
  tar jxvf gtk+-2.18.5.tar.bz2
  cd gtk+-2.18.5
  # Unmaintained makefiles ftw...
  sed -e 's@.exe : ../gtk/gtk-win32-$(GTK_VER).lib @.exe : @' tests/makefile.msc >tests/makefile.msc.new
  mv -v tests/makefile.msc.new tests/makefile.msc
  sed -e '/\\wntab/d' -e '/\\wtkit/d' gdk/makefile.msc >gdk/makefile.msc.new
  mv -v gdk/makefile.msc.new gdk/makefile.msc
  OLDPATH="$PATH"
  PATH="`pwd`/gdk-pixbuf:$PATH"  # so gtk-update-icon-cache can actually run during the build
  nmake -f makefile.msc
  PATH="$OLDPATH"
  OLDPATH=
  cd gdk-pixbuf
  mkdir -pv ../../../include/gdk-pixbuf
  cp -v gdk-pixbuf.h gdk-pixbuf-core.h gdk-pixbuf-transform.h gdk-pixbuf-io.h \
    gdk-pixbuf-animation.h gdk-pixbuf-simple-anim.h gdk-pixbuf-loader.h \
    gdk-pixbuf-enum-types.h gdk-pixbuf-marshal.h gdk-pixbuf-features.h \
    gdk-pixdata.h ../../../include/gdk-pixbuf
  manifestify gdk-pixbuf-csource.exe
  cp -v libgdk_pixbuf-2.0-0.dll gdk-pixbuf-csource.exe ../../../bin
  cp -v gdk_pixbuf-2.0.lib ../../../lib
  cd ../gdk
  mkdir -pv ../../../include/gdk
  cp -v gdk.h gdkapplaunchcontext.h gdkcairo.h gdkcolor.h gdkcursor.h gdkdisplay.h \
    gdkdisplaymanager.h gdkdnd.h gdkdrawable.h gdkevents.h gdkfont.h gdkgc.h \
    gdki18n.h gdkimage.h gdkinput.h gdkkeys.h gdkkeysyms.h gdkpango.h gdkpixbuf.h \
    gdkpixmap.h gdkprivate.h gdkproperty.h gdkregion.h gdkrgb.h gdkscreen.h \
    gdkselection.h gdkspawn.h gdktestutils.h gdktypes.h gdkvisual.h gdkwindow.h \
    win32/gdkwin32.h gdkenumtypes.h ../../../include/gdk
  cp -v gdkconfig.h ../../../include
  cp -v libgdk-win32-2.0-0.dll ../../../bin
  cp -v gdk-win32-2.0.lib ../../../lib
  cd ../gtk
  mkdir -pv ../../../include/gtk
  cp -v gtk.h gtkaboutdialog.h gtkaccelgroup.h gtkaccellabel.h gtkaccelmap.h \
    gtkaccessible.h gtkaction.h gtkactiongroup.h gtkactivatable.h gtkadjustment.h \
    gtkalignment.h gtkarrow.h gtkaspectframe.h gtkassistant.h gtkbbox.h gtkbin.h \
    gtkbindings.h gtkbox.h gtkbuilder.h gtkbuildable.h gtkbutton.h gtkcalendar.h \
    gtkcelleditable.h gtkcelllayout.h gtkcellrenderer.h gtkcellrendereraccel.h \
    gtkcellrenderercombo.h gtkcellrendererpixbuf.h gtkcellrendererprogress.h \
    gtkcellrendererspin.h gtkcellrenderertext.h gtkcellrenderertoggle.h \
    gtkcellview.h gtkcheckbutton.h gtkcheckmenuitem.h gtkclipboard.h \
    gtkcolorbutton.h gtkcolorsel.h gtkcolorseldialog.h gtkcombobox.h \
    gtkcomboboxentry.h gtkcontainer.h gtkcurve.h gtkdebug.h gtkdialog.h gtkdnd.h \
    gtkdrawingarea.h gtkeditable.h gtkentry.h gtkentrybuffer.h gtkentrycompletion.h \
    gtkenums.h gtkeventbox.h gtkexpander.h gtkfilechooser.h gtkfilechooserbutton.h \
    gtkfilechooserdialog.h gtkfilechooserwidget.h gtkfilefilter.h gtkfixed.h \
    gtkfontbutton.h gtkfontsel.h gtkframe.h gtkgamma.h gtkgc.h gtkhandlebox.h \
    gtkhbbox.h gtkhbox.h gtkhpaned.h gtkhruler.h gtkhscale.h gtkhscrollbar.h \
    gtkhseparator.h gtkhsv.h gtkiconfactory.h gtkicontheme.h gtkiconview.h \
    gtkimage.h gtkimagemenuitem.h gtkimcontext.h gtkimcontextsimple.h gtkimmodule.h \
    gtkimmulticontext.h gtkinfobar.h gtkinputdialog.h gtkinvisible.h gtkitem.h \
    gtklabel.h gtklayout.h gtklinkbutton.h gtkliststore.h gtkmain.h gtkmenu.h \
    gtkmenubar.h gtkmenuitem.h gtkmenushell.h gtkmenutoolbutton.h \
    gtkmessagedialog.h gtkmisc.h gtkmodules.h gtkmountoperation.h gtknotebook.h \
    gtkobject.h gtkorientable.h gtkpagesetup.h gtkpaned.h gtkpapersize.h gtkplug.h \
    gtkprintcontext.h gtkprintoperation.h gtkprintoperationpreview.h \
    gtkprintsettings.h gtkprivate.h gtkprogressbar.h gtkradioaction.h \
    gtkradiobutton.h gtkradiomenuitem.h gtkradiotoolbutton.h gtkrange.h gtkrc.h \
    gtkrecentaction.h gtkrecentchooser.h gtkrecentchooserdialog.h \
    gtkrecentchoosermenu.h gtkrecentchooserwidget.h gtkrecentfilter.h \
    gtkrecentmanager.h gtkruler.h gtkscale.h gtkscalebutton.h gtkscrollbar.h \
    gtkscrolledwindow.h gtkselection.h gtkseparator.h gtkseparatormenuitem.h \
    gtkseparatortoolitem.h gtkshow.h gtksettings.h gtksizegroup.h gtksocket.h \
    gtkspinbutton.h gtkstatusbar.h gtkstatusicon.h gtkstock.h gtkstyle.h gtktable.h \
    gtktearoffmenuitem.h gtktestutils.h gtktextbuffer.h gtktextbufferrichtext.h \
    gtktextchild.h gtktextdisplay.h gtktextiter.h gtktextmark.h gtktexttag.h \
    gtktexttagtable.h gtktextview.h gtktoggleaction.h gtktogglebutton.h \
    gtktoggletoolbutton.h gtktoolbar.h gtktoolbutton.h gtktoolitem.h gtktoolshell.h \
    gtktooltip.h gtktreednd.h gtktreemodel.h gtktreemodelfilter.h \
    gtktreemodelsort.h gtktreeselection.h gtktreesortable.h gtktreestore.h \
    gtktreeview.h gtktreeviewcolumn.h gtktypeutils.h gtkuimanager.h gtkvbbox.h \
    gtkvbox.h gtkviewport.h gtkvolumebutton.h gtkvpaned.h gtkvruler.h gtkvscale.h \
    gtkvscrollbar.h gtkvseparator.h gtkwidget.h gtkwindow.h gtktext.h gtktree.h \
    gtktreeitem.h gtkclist.h gtkcombo.h gtkctree.h gtkfilesel.h gtkitemfactory.h \
    gtklist.h gtklistitem.h gtkoldeditable.h gtkoptionmenu.h gtkpixmap.h \
    gtkpreview.h gtkprogress.h gtksignal.h gtktipsquery.h gtktooltips.h \
    gtktextlayout.h gtkmarshal.h gtktypebuiltins.h gtkversion.h \
      ../../../include/gtk
  manifestify gtk-query-immodules-2.0.exe
  manifestify gtk-update-icon-cache.exe
  cp -v libgtk-win32-2.0-0.dll gtk-query-immodules-2.0.exe gtk-update-icon-cache.exe ../../../bin
  cp -v gtk-win32-2.0.lib ../../../lib
  cd ../..  # TODO: anything else, if necessary
  $RM_RF gtk+-2.18.5
fi

$RM_RF glib

if ! test -f ../lib/rsvg-2.lib; then
  tar jxvf librsvg-2.26.0.tar.bz2
  cd librsvg-2.26.0

# Another autotools-only package, another shell script hack-o-rama.
# TODO: libgsf/libcroco support?  some svgs might need it...
# --- cut here ---
#!/bin/sh -e
RSVG_SOURCES="rsvg-affine.c librsvg-features.c rsvg-bpath-util.c \
  rsvg-css.c rsvg-defs.c rsvg-image.c rsvg-paint-server.c rsvg-path.c \
  rsvg-base-file-util.c rsvg-filter.c rsvg-marker.c rsvg-mask.c rsvg-shapes.c \
  rsvg-structure.c rsvg-styles.c rsvg-text.c rsvg-cond.c rsvg-base.c \
  librsvg-enum-types.c rsvg-cairo-draw.c rsvg-cairo-render.c rsvg-cairo-clip.c \
  rsvg.c rsvg-gobject.c rsvg-file-util.c"

RSVG_HEADERS="rsvg-css.h rsvg-defs.h rsvg-image.h rsvg-paint-server.h \
  rsvg-path.h rsvg-private.h rsvg-filter.h rsvg-marker.h rsvg-mask.h \
  rsvg-shapes.h rsvg-structure.h rsvg-styles.h rsvg-bpath-util.h rsvg-text.h \
  rsvg-cairo-draw.h rsvg-cairo-render.h rsvg-cairo-clip.h"

(echo EXPORTS; grep -v EXPORTS librsvg.def) >librsvg.def.new
mv -v librsvg.def.new librsvg.def
sed -e 's/#undef VERSION/#define VERSION "2.26.0"/' config.h.in >config.h
cl -I . -I ../../include/glib-2.0 -I ../../lib/glib-2.0/include \
  -Felibrsvg-2.0-0.dll $RSVG_SOURCES -link -dll -def:librsvg.def -implib:rsvg-2.lib \
  glib-2.0.lib gobject-2.0.lib xml2.lib cairo.lib gdk_pixbuf-2.0.lib \
  pango-1.0.lib pangocairo-1.0.lib
cl -I . -I ../../include/glib-2.0 -I ../../lib/glib-2.0/include \
  -Fersvg-convert.exe rsvg-convert.c rsvg-2.lib glib-2.0.lib \
  gobject-2.0.lib gthread-2.0.lib cairo.lib
manifestify rsvg-convert.exe
cl -I . -I ../../include/glib-2.0 -I ../../lib/glib-2.0/include \
  -Fersvg-view.exe test-display.c rsvg-2.lib gtk-win32-2.0.lib \
  gdk-win32-2.0.lib gdk_pixbuf-2.0.lib gobject-2.0.lib \
  gthread-2.0.lib glib-2.0.lib
manifestify rsvg-view.exe
mkdir -pv ../../include/librsvg
cp -v rsvg.h rsvg-cairo.h librsvg-features.h librsvg-enum-types.h ../../include/librsvg
cp -v librsvg-2.0-0.dll rsvg-convert.exe rsvg-view.exe ../../bin
cp -v rsvg-2.lib ../../lib
# --- cut here ---

  cd ..
  $RM_RF librsvg-2.26.0
fi

if ! test -f ../lib/sigc-2.0.lib; then
  tar jxvf libsigc++-2.2.4.2.tar.bz2
  cd libsigc++-2.2.4.2/MSVC_Net2008
  cat >fixgccdef.sed <<"EOF"
/^#define SIGC_GCC_TEMPLATE_SPECIALIZATION_OPERATOR_OVERLOAD/ {i\
#ifdef __GNUC__
a\
#endif
}
EOF
  sed -f fixgccdef.sed sigc++config.h >sigc++config.h.new
  mv -v sigc++config.h.new sigc++config.h
  $RM_RF fixgccdef.sed
  vcbuild -useenv libsigc++2.vcproj 'Release|Win32'
  mkdir -pv ../../../include/sigc++
  cp -v sigc++config.h ../../../include
  for file in sigc++.h bind.h bind_return.h connection.h object.h reference_wrapper.h \
    retype_return.h signal_base.h trackable.h type_traits.h visit_each.h \
    adaptors/adaptors.h adaptors/bound_argument.h adaptors/lambda/lambda.h \
    functors/functors.h functors/slot_base.h signal.h slot.h method_slot.h \
    object_slot.h class_slot.h hide.h retype.h limit_reference.h \
    functors/functor_trait.h functors/slot.h functors/ptr_fun.h functors/mem_fun.h \
    adaptors/deduce_result_type.h adaptors/adaptor_trait.h adaptors/bind.h \
    adaptors/bind_return.h adaptors/retype_return.h adaptors/hide.h adaptors/retype.h \
    adaptors/compose.h adaptors/exception_catch.h adaptors/lambda/base.h \
    adaptors/lambda/select.h adaptors/lambda/operator.h adaptors/lambda/group.h; do
      mkdir -pv ../../../include/sigc++/"`dirname "$file"`"
      cp -v ../sigc++/"$file" ../../../include/sigc++/"`dirname "$file"`"
  done
  cp -v Release/sigc-vc90-2_0.dll ../../../bin
  cp -v Release/sigc-vc90-2_0.lib ../../../lib/sigc-2.0.lib
  cd ../..
  $RM_RF libsigc++-2.2.4.2
fi

if ! test -f ../lib/glibmm-2.4.lib; then
  tar jxvf glibmm-2.22.1.tar.bz2
  cd glibmm-2.22.1/MSVC_Net2008
  cat >fixrc.sed <<"EOF"
s/@G[A-Z]*MM_MAJOR_VERSION@/2/g
s/@G[A-Z]*MM_MINOR_VERSION@/22/g
s/@G[A-Z]*MM_MICRO_VERSION@/1/g
s/@PACKAGE_VERSION@/2.22.1/g
s/@GLIBMM_MODULE_NAME@/glibmm-vc90-2_4/g
s/@GIOMM_MODULE_NAME@/giomm-vc90-2_4/g
EOF
  for file in */*.rc.in; do
    sed -f fixrc.sed "$file" >"`echo "$file" | sed -e 's/\.in$//'`"
  done
  $RM_RF fixrc.sed
  for file in */*.vcproj */*/*.vcproj; do
    sed -e 's/sigc-vc90-2_0.lib/sigc-2.0.lib/g' "$file" >"$file".new
    mv -v "$file".new "$file"
  done
  cat >fixiostream.sed <<"EOF"
/<\/Files>/i\
<File RelativePath="..\\..\\gio\\giomm\\fileiostream.cc" />\
<File RelativePath="..\\..\\gio\\giomm\\iostream.cc" />
EOF
  sed -f fixiostream.sed giomm/giomm.vcproj >giomm/giomm.vcproj.new
  mv -v giomm/giomm.vcproj.new giomm/giomm.vcproj
  $RM_RF fixiostream.sed
  OLDINCLUDE="$INCLUDE"
  INCLUDE="`cd ../../../include/glib-2.0 && cmd //c echo $(pwd)`;`cd ../../../lib/glib-2.0/include && cmd //c echo $(pwd)`;$INCLUDE"
  vcbuild -useenv glibmm.sln 'Release|Win32'
  INCLUDE="$OLDINCLUDE"
  OLDINCLUDE=
  cp -v glibmm/glibmmconfig.h giomm/giommconfig.h ../../../include
  cp -v ../glib/glibmm.h ../gio/giomm.h ../../../include
  mkdir -pv ../../../include/glibmm/private
  cp -v ../glib/glibmm/*.h ../../../include/glibmm
  cp -v ../glib/glibmm/private/*.h ../../../include/glibmm/private
  mkdir -pv ../../../include/giomm/private
  cp -v ../gio/giomm/*.h ../../../include/giomm
  cp -v ../gio/giomm/private/*.h ../../../include/giomm/private
  cp -v glibmm/Release/glibmm-vc90-2_4.dll giomm/Release/giomm-vc90-2_4.dll ../../../bin
  cp -v giomm/Release/giomm-vc90-2_4.lib ../../../lib/giomm-2.4.lib
  cp -v glibmm/Release/glibmm-vc90-2_4.lib ../../../lib/glibmm-2.4.lib
  cd ../..
  $RM_RF glibmm-2.22.1
fi

if ! test -f ../lib/xml++-2.6.lib; then
  tar jxvf libxml++-2.26.1.tar.bz2
  cd libxml++-2.26.1/MSVC_Net2008
  for file in */*.vcproj */*/*.vcproj; do
    sed -e 's/libxml2.lib/xml2.lib/g' -e 's/glibmm-vc90-2_4.lib/glibmm-2.4.lib/g' "$file" >"$file".new
    mv -v "$file".new "$file"
  done
  sed -e 's@^#include "afxres.h"@#include <windows.h>@' libxml++/libxml++.rc >libxml++/libxml++.rc.new
  mv -v libxml++/libxml++.rc.new libxml++/libxml++.rc
  OLDINCLUDE="$INCLUDE"
  INCLUDE="`cd ../../../include/glib-2.0 && cmd //c echo $(pwd)`;`cd ../../../lib/glib-2.0/include && cmd //c echo $(pwd)`;$INCLUDE"
  vcbuild -useenv libxml++.sln 'Release|Win32'
  INCLUDE="$OLDINCLUDE"
  OLDINCLUDE=
  cp -v libxml++/libxml++config.h ../../../include
  cd ../libxml++
  mkdir -pv ../../../include/libxml++
  cp -v libxml++.h attribute.h dtd.h document.h noncopyable.h keepblanks.h schema.h ../../../include/libxml++
  for dir in exceptions parsers nodes validators; do
    mkdir -pv ../../../include/libxml++/"$dir"
    cp -v "$dir"/*.h ../../../include/libxml++/"$dir"
  done
  cd ../MSVC_Net2008
  cp -v libxml++/Release/xml++-vc90-2_6.dll ../../../bin
  cp -v libxml++/Release/xml++-vc90-2_6.lib ../../../lib/xml++-2.6.lib
  cd ../..
  $RM_RF libxml++-2.26.1
fi

if ! test -f ../lib/glew32.lib; then
  tar zxvf glew-1.5.1-src.tgz
  cd glew
  # VS2008 vcbuild doesn't like upgrading dsw/dsp projects...
  cat >CMakeLists.txt <<"EOF"
include_directories(include)
add_library(glew32 SHARED src/glew.c build/vc6/glew.rc)
target_link_libraries(glew32 opengl32 glu32)
set_target_properties(glew32 PROPERTIES COMPILE_DEFINITIONS GLEW_BUILD)
add_executable(glewinfo src/glewinfo.c build/vc6/glewinfo.rc)
target_link_libraries(glewinfo glew32)
add_executable(visualinfo src/visualinfo.c build/vc6/visualinfo.rc)
target_link_libraries(visualinfo glew32)
EOF
  cmake -DCMAKE_BUILD_TYPE=Release .
  nmake
  cp -av include/GL ../../include
  cp -v glew32.dll glewinfo.exe visualinfo.exe ../../bin
  cp -v glew32.lib ../../lib
  cd ..
  $RM_RF glew
fi

#if ! test -f ../lib/Magick++.lib; then
#  7z x -y ImageMagick-6.5.8-6.7z
#  cd ImageMagick-6.5.8
#  # The build system works, but requires GUI interaction, which we don't want.
#  # (Also, let's use our own libraries rather than IM's internal copies!)
#  # Be sure to alter this if extra libs are added to this environment that
#  # IM can make use of.
#  cat >magick-config.sed <<"EOF"
#/MAGICKCORE_X11_DELEGATE/s@.*@// &@
#/MAGICKCORE_BZLIB_DELEGATE/s@.*@// &@
#/MAGICKCORE_JBIG_DELEGATE/s@.*@// &@
#/MAGICKCORE_JP2_DELEGATE/s@.*@// &@
#/MAGICKCORE_LCMS_DELEGATE/s@.*@// &@
#/MAGICKCORE_WMFLITE_DELEGATE/s@.*@// &@
#EOF
#  sed -f magick-config.sed <VisualMagick/magick/magick-config.h.in >magick/magick-config.h
#  $RM_RF magick-config.sed
#  cat >CMakeLists.txt <<"EOF"
#add_definitions(-DNDEBUG -D_WINDOWS -DWIN32 -D_VISUALC_ -DNeedFunctionPrototypes -D_DLL)
#include_directories(.)
#subdirs(coders magick Magick++/lib utilities wand)
#EOF
#  cat >coders/CMakeLists.txt <<"EOF"
#macro(IM_CODER coder_name)
#  add_library(IM_MOD_RL_${coder_name}_ MODULE ${coder_name}.c)
#  target_link_libraries(IM_MOD_RL_${coder_name}_ MagickCore ${ARGN})
#endmacro(IM_CODER)
#
## Coders that don't need any external libraries.
#foreach(nolib_coder art avs bmp braille cals caption cin cip clip clipboard
#  cmyk cut dcm dds dib djvu dng dot dps dpx emf ept exr fax fits fpx gif
#  gradient gray hald histogram hrz html icon info inline ipl jbig jp2
#  label magick map mat matte meta miff mono mpc mpeg mpr mtv mvg null otb
#  palm pattern pcd pcl pcx pdb pdf pict pix plasma pnm preview ps2 ps3 ps
#  psd pwp raw rgb rla rle scr sct sfw sgi stegano sun tga thumbnail
#  tile tim ttf txt uil url uyvy vicar vid viff wbmp wmf wpg x xbm xc xcf xpm
#  xps xtrn xwd ycbcr yuv)
#    IM_CODER(${nolib_coder})
#endforeach(nolib_coder)
#
## Ones that do need external libs.
#IM_CODER(jpeg jpeg)
#IM_CODER(msl xml2)
#IM_CODER(png libpng)
#IM_CODER(svg xml2)
#IM_CODER(tiff tiff)
#IM_CODER(url xml2)
#EOF
#  cat >magick/CMakeLists.txt <<"EOF"
#add_definitions(-D_MT -D_MAGICKMOD_ -D_MAGICKLIB_)
#add_library(MagickCore SHARED animate.c annotate.c artifact.c attribute.c
#  blob.c cache.c cache-view.c cipher.c client.c coder.c color.c colormap.c
#  colorspace.c compare.c composite.c compress.c configure.c constitute.c
#  decorate.c delegate.c deprecate.c display.c distort.c draw.c effect.c
#  enhance.c exception.c fourier.c fx.c gem.c geometry.c hashmap.c histogram.c
#  identify.c image.c layer.c list.c locale.c log.c magic.c magick.c matrix.c
#  memory.c mime.c module.c monitor.c montage.c morphology.c option.c paint.c
#  pixel.c policy.c PreRvIcccm.c prepress.c property.c profile.c quantize.c
#  quantum.c quantum-export.c quantum-import.c random.c registry.c resample.c
#  resize.c resource.c segment.c semaphore.c shear.c signature.c splay-tree.c
#  static.c statistic.c stream.c string.c thread.c timer.c token.c transform.c
#  threshold.c type.c utility.c version.c widget.c xml-tree.c xwindow.c
#  nt-base.c nt-feature.c
#
#  ImageMagick.h MagickCore.h animate.h animate-private.h annotate.h
#  api.h artifact.h attribute.h blob.h blob-private.h cache.h cache-private.h
#  cache-view.h cipher.h client.h coder.h color.h color-private.h colormap.h
#  colormap-private.h colorspace.h colorspace-private.h compare.h compress.h
#  configure.h constitute.h composite.h composite-private.h decorate.h
#  delegate.h delegate-private.h deprecate.h display.h display-private.h
#  distort.h draw.h draw-private.h effect.h enhance.h exception.h
#  exception-private.h fourier.h fx.h fx-private.h gem.h geometry.h hashmap.h
#  histogram.h identify.h image.h image-private.h layer.h list.h locale_.h
#  log.h mac.h magic.h magick-config.h magick-type.h magick.h matrix.h
#  memory_.h methods.h mime.h module.h monitor.h monitor-private.h montage.h
#  morphology.h nt-base.h nt-feature.h option.h paint.h pixel.h pixel-private.h
#  policy.h PreRvIcccm.h prepress.h property.h profile.h quantize.h quantum.h
#  quantum-private.h random_.h random-private.h registry.h resample.h
#  resample-private.h resize.h resize-private.h resource_.h segment.h
#  semaphore.h shear.h signature.h signature-private.h splay-tree.h static.h
#  statistic.h stream.h stream-private.h string_.h studio.h thread_.h
#  thread-private.h timer.h token.h token-private.h transform.h threshold.h
#  type.h utility.h version.h vms.h widget.h xml-tree.h xwindow.h nt-base.h
#  nt-feature.h
#
#  ImageMagick.rc)
#target_link_libraries(MagickCore zdll freetype)
#EOF
#  cat >Magick++/lib/CMakeLists.txt <<"EOF"
#include_directories(.)
#add_library(MagickPlusPlus SHARED Blob.cpp BlobRef.cpp CoderInfo.cpp Color.cpp
#  Drawable.cpp Exception.cpp Functions.cpp Geometry.cpp Image.cpp ImageRef.cpp
#  Montage.cpp Options.cpp Pixels.cpp STL.cpp Thread.cpp TypeMetric.cpp
#
#  Magick++.h Magick++/Blob.h Magick++/BlobRef.h Magick++/CoderInfo.h
#  Magick++/Color.h Magick++/Drawable.h Magick++/Exception.h Magick++/Functions.h
#  Magick++/Geometry.h Magick++/Image.h Magick++/ImageRef.h Magick++/Include.h
#  Magick++/Montage.h Magick++/Options.h Magick++/Pixels.h Magick++/STL.h
#  Magick++/Thread.h Magick++/TypeMetric.h
#
#  ../../magick/ImageMagick.rc)
#target_link_libraries(MagickPlusPlus MagickCore MagickWand)
#set_target_properties(MagickPlusPlus PROPERTIES OUTPUT_NAME Magick++)
#EOF
#  cat >utilities/CMakeLists.txt <<"EOF"
#macro(IM_UTIL util_name)
#  add_executable(${util_name} ${util_name}.c ../magick/ImageMagick.rc)
#  target_link_libraries(${util_name} MagickCore MagickWand)
#endmacro(IM_UTIL)
#
#foreach(util animate compare composite conjure convert display identify
#  import mogrify montage stream)
#    IM_UTIL(${util})
#endforeach(util)
#EOF
#  cat >wand/CMakeLists.txt <<"EOF"
#add_definitions(-D_MT -D_MAGICKMOD_ -D_MAGICKLIB_)
#add_library(MagickWand SHARED animate.c compare.c composite.c conjure.c
#  convert.c deprecate.c display.c drawing-wand.c identify.c import.c
#  magick-image.c magick-property.c magick-wand.c mogrify.c montage.c
#  pixel-iterator.c pixel-view.c pixel-wand.c stream.c wand.c
#
#  MagickWand.h animate.h compare.h composite.h conjure.h convert.h deprecate.h
#  display.h drawing-wand.h identify.h import.h magick-image.h magick-property.h
#  magick-wand.h magick-wand-private.h mogrify.h mogrify-private.h montage.h
#  pixel-iterator.h pixel-view.h pixel-wand.h pixel-wand-private.h stream.h
#  studio.h wand.h
#
#  ../VisualMagick/wand/wand.rc)
#target_link_libraries(MagickWand MagickCore)
#EOF
#  cmake -DCMAKE_BUILD_TYPE=Release .
#  nmake
#  # TODO: Place the coders in their own folder so they don't clutter up bin.
#  mkdir -pv ../../include/magick ../../include/wand ../../include/Magick++
#  cp -v magick/ImageMagick.h magick/MagickCore.h magick/PreRvIcccm.h magick/animate.h \
#  magick/annotate.h magick/api.h magick/artifact.h magick/attribute.h magick/blob.h \
#  magick/cache.h magick/cache-view.h magick/cipher.h magick/client.h magick/coder.h \
#  magick/color.h magick/colormap.h magick/colorspace.h magick/compare.h magick/composite.h \
#  magick/compress.h magick/configure.h magick/constitute.h magick/decorate.h \
#  magick/delegate.h magick/deprecate.h magick/display.h magick/distort.h magick/draw.h \
#  magick/effect.h magick/enhance.h magick/exception.h magick/fourier.h magick/fx.h \
#  magick/gem.h magick/geometry.h magick/hashmap.h magick/histogram.h magick/identify.h \
#  magick/image.h magick/layer.h magick/list.h magick/locale_.h magick/log.h \
#  magick/magic.h magick/magick.h magick/magick-config.h magick/magick-type.h \
#  magick/matrix.h magick/memory_.h magick/methods.h magick/mime.h magick/module.h \
#  magick/monitor.h magick/montage.h magick/morphology.h magick/option.h magick/paint.h \
#  magick/pixel.h magick/policy.h magick/prepress.h magick/profile.h magick/property.h \
#  magick/quantize.h magick/quantum.h magick/random_.h magick/registry.h magick/resample.h \
#  magick/resize.h magick/resource_.h magick/segment.h magick/semaphore.h magick/shear.h \
#  magick/signature.h magick/splay-tree.h magick/statistic.h magick/stream.h \
#  magick/string_.h magick/timer.h magick/token.h magick/transform.h magick/threshold.h \
#  magick/type.h magick/utility.h magick/version.h magick/widget.h magick/xml-tree.h \
#  magick/xwindow.h ../../include/magick
#  cp -v wand/MagickWand.h wand/animate.h wand/compare.h wand/composite.h wand/conjure.h \
#  wand/convert.h wand/deprecate.h wand/display.h wand/drawing-wand.h wand/identify.h \
#  wand/import.h wand/magick-image.h wand/magick-property.h wand/magick-wand.h \
#  wand/magick_wand.h wand/mogrify.h wand/montage.h wand/pixel-iterator.h \
#  wand/pixel-view.h wand/pixel-wand.h wand/stream.h ../../include/wand
#  cp -v Magick++/lib/Magick++.h ../../include
#  cp -v Magick++/lib/Magick++/Blob.h Magick++/lib/Magick++/CoderInfo.h \
#  Magick++/lib/Magick++/Color.h Magick++/lib/Magick++/Drawable.h \
#  Magick++/lib/Magick++/Exception.h Magick++/lib/Magick++/Geometry.h \
#  Magick++/lib/Magick++/Image.h Magick++/lib/Magick++/Include.h \
#  Magick++/lib/Magick++/Montage.h Magick++/lib/Magick++/Pixels.h \
#  Magick++/lib/Magick++/STL.h Magick++/lib/Magick++/TypeMetric.h \
#    ../../include/Magick++
#  cp -v magick/MagickCore.dll wand/MagickWand.dll Magick++/lib/Magick++.dll coders/IM_MOD_RL_*_.dll utilities/*.exe ../../bin
#  cp -v magick/MagickCore.lib wand/MagickWand.lib Magick++/lib/Magick++.lib ../../lib
#  cd ..
#  $RM_RF ImageMagick-6.5.8
#fi

# So we've dealt with horrible or uncooperative build systems, but now we've
# come to the downright impossible.  Since ffmpeg is never going to compile
# under MSVC for the foreseeable future, just bite the bullet and use binaries.
if ! test -f ../lib/avcodec.lib; then
  mkdir -pv ffbinaries
  7z x -y -offbinaries ffmpeg-r20817-swscale-r29978-mingw32-shared.7z
  7z x -y -offbinaries ffmpeg-r20817-swscale-r29978-mingw32-shared-dev.7z
  cp -av ffbinaries/include/lib* ../include
  cp -v ffbinaries/bin/* ../bin
  cp -v ffbinaries/lib/*.lib ../lib
  $RM_RF ffbinaries
fi

if ! test -f ../lib/SDL_image.lib; then
  tar zxvf SDL_image-1.2.10.tar.gz
  cd SDL_image-1.2.10
  unzip -o VisualC.zip
  cd VisualC
  sed -e '/PreprocessorDefinitions/s/LOAD_[A-Z]*_DYNAMIC=\\&quot;[-a-z0-9.]*\\&quot;;//g' -e 's/SDL.lib/& jpeg.lib libpng.lib tiff.lib/g' SDL_image.vcproj >SDL_image.vcproj.new
  mv -v SDL_image.vcproj.new SDL_image.vcproj
  OLDINCLUDE="$INCLUDE"
  INCLUDE="../../../include/SDL;$INCLUDE"
  vcbuild -useenv SDL_image.sln 'Release|Win32'
  INCLUDE="$OLDINCLUDE"
  OLDINCLUDE=
  cp -v ../SDL_image.h ../../../include/SDL
  cp -v Release/SDL_image.dll ../../../bin
  cp -v Release/SDL_image.lib ../../../lib
  cd ../..
  $RM_RF SDL_image-1.2.10
fi

$RM_RF dummy.vcproj dummy-cygpath