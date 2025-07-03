# Contributing

## Preparations

Be sure not to have multiple Performous versions installed at the same time. It may seem to work, but the graphics may display incorrectly due to wrong theme files being used. To prevent that, clean up old files:

```bash
rm -rf /usr/local/bin/performous /usr/local/share/games/performous/
```

## Dependencies

If you happen to be running one of these, please start by installing the following packages. If not, just skip this section and follow the instructions. We try to keep these lists accurate and up-to-date with the development version. If you find that something is missing or that there are unnecessary package installs, please fix it.

Gettext (localization), Help2Man (UNIX manual pages), OpenCV (webcam support) and PortMidi (MIDI drum support) are optional and can be left out if no support for these functions is required.

### Debian and Ubuntu

```bash
sudo apt-get install git-core cmake build-essential gettext help2man \
   libepoxy-dev libsdl2-dev libcairo2-dev libpango1.0-dev librsvg2-dev \
   libboost-all-dev libavcodec-dev libavformat-dev libswscale-dev libswresample-dev \
   libpng-dev libjpeg-dev libxml++2.6-dev portaudio19-dev \
   libopencv-dev libportmidi-dev libcpprest-dev nlohmann-json3-dev libfmt-dev
```

Notice: Dependency problems may prevent installation of portaudio19-dev. At least with Ubuntu 13.04 this can be solved by first installing libjack-jackd2-dev, even though that package is not actually needed for Performous.

Alternatively, you can do:

```bash
sudo apt-get build-dep performous
```

which installs all the build dependencies for the version in the repositories. It might not be completely accurate for the current git version, but should get you pretty far.

Note: when building the webserver-branch you need CPP-netlib version `0.11.2` which can be downloaded from here: https://github.com/cpp-netlib/cpp-netlib/archive/cpp-netlib-0.11.2-final.zip and unpack the folder named "boost" into /usr/include.
if done correctly you should have the file: /usr/include/boost/network/protocol/http/server.hpp among some others.

### Gentoo

```bash
sudo emerge boost git cmake help2man glew libsdl2 ffmpeg portaudio libxmlpp \
   opencv portmidi
```

### Fedora

You need [http://rpmfusion.org/Configuration RPM Fusion Free] repository for ffmpeg.
It's best to fetch and install this first, as the package-install below depends on it.

```bash
yum install git cmake gcc-c++ gettext cairo-devel librsvg2-devel libsigc++20-devel \
   glibmm24-devel libxml++-devel boost-devel SDL2-devel libepoxy-devel ffmpeg-devel \
   portaudio-devel help2man redhat-lsb opencv-devel portmidi-devel libjpeg-turbo-devel \
   pango-devel jsoncpp-devel fmt-devel
```

If you also plan to run unit tests, further packages are required.  
(This is **not** needed just to play Performous)
```bash
yum install gtest-devel gmock-devel
```

### MacOS

These instructions will walk you through building Performous for macOS (10.9+) and bundling and packaging it inside a DMG ready to install. By default these instructions build the internal webserver, the extra tools (such as Singstar extractor; note, not all are currently working in macOS, even if they all do build successfully), and support for webcam (via OpenCV)

* These instruction have been tested with macOS 10.12.5 and XCode 8.3.3.

1.  Install macports first, clean installation is preferred to avoid conflicts.
2.  Update macports and its port list via 

```bash
sudo port selfupdate
```

* Most dependencies required to use the provided build script must be installed using MacPorts. A couple of them (marked with asterisks, below) are not available in this manner and will have to be obtained and built separately.
You can use the default 
  sudo port install <dependency_name>
to install the dependencies and it's reported to work. However, official macOS releases are built using the following configuration parameters:

```bash
 sudo /opt/local/bin/port -vsf install <package_name> -universal configure.compiler="clang" \ 
 configure.cxx_stdlib="libc++" configure.macosx_deployment_target="10.9" \
 configure.sdkroot="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk" \
 configure.cxxflags="-std=c++11 -mmacosx-version-min=10.9 -m64 -arch x86_64 -stdlib=libc++ \ 
 -Wl,-headerpad_max_install_names" configure.cflags="-m64 -arch x86_64 -mmacosx-version-min=10.9 \
 -Wl,-headerpad_max_install_names" configure.ldflags="-m64 -arch x86_64 -stdlib=libc++ \ 
 -headerpad_max_install_names -mmacosx-version-min=10.9"
```

and also these, in `~/.macports/macports.conf` (largely they're the same things as the commandline parameters, but Macports is sometimes... well, special, in deciding which things can be configured via command-line and which cannot.

```bash
  build_arch          	x86_64
  cxx_stdlib		libc++
  configure.cxx_stdlib	libc++
  macosx_deployment_target 10.9
  configure.cxxflags  "-mmacosx-version-min=10.9 -m64 -arch x86_64 -stdlib=libc++ -Wl,-headerpad_max_install_names"
  configure.cflags  "-mmacosx-version-min=10.9 -m64 -arch x86_64 -Wl,-headerpad_max_install_names"
  configure.ldflags "-m64 -arch x86_64 -stdlib=libc++ -headerpad_max_install_names -mmacosx-version-min=10.9"
  cxxflags  "-mmacosx-version-min=10.9 -m64 -arch x86_64 -stdlib=libc++ -Wl,-headerpad_max_install_names"
  cflags  "-mmacosx-version-min=10.9 -m64 -arch x86_64 -Wl,-headerpad_max_install_names"
  ldflags "-m64 -arch x86_64 -stdlib=libc++ -headerpad_max_install_names -mmacosx-version-min=10.9"
  configure.sdkroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
  sdkroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
  universal_archs     	x86_64 noarch
  buildmakejobs       	0
```

3. Dependencies

* npm
* boost
* libxmlxx2
* libsdl2
* ffmpeg
* cmake
* jsoncpp
* help2man
* libepoxy
* librsvg
* portaudio
* portmidi
* opencv
* dylibbundler (*)
* asio-c++ (*)
* cppnetlib (*)

   * Note: Libraries that are required by other dependencies are absent from this list (since they will be installed automatically) in order to simplify the instructions.
   * Note: Dependencies marked with an asterisk are not available or are outdated in Macports and should be obtained elsewhere.

4. Download dylibbundler, asioc++ and cpp-netlib version 0.11.2

* Download dylibbundler from https://github.com/auriamg/macdylibbundler/ and install it according to the README.
* Download asioc++ from http://think-async.com/Asio/Download, extract it, and open a terminal. Navigate to the folder where the asio folder is, and type the following:

```bash
 mkdir asio-build && cd asio-build
  ASIO_STANDALONE=1 CXXFLAGS="-std=c++11 -stdlib=libc++" CC="/usr/bin/clang" CXX="/usr/bin/clang++" ../asio-1.10.6/configure --prefix=/opt/local --with-boost=no
 make check && sudo make install
```

* Note: Make sure the path to configure is correct, here I am using the default folder to which asioc++ is extracted. 

* Download cpp-netlib 0.11.2 from https://github.com/cpp-netlib/cpp-netlib/releases/tag/cpp-netlib-0.11.2-final
* Extract it, and once again open a terminal. Navigate to where the cpp-netlib-0.11.2-final folder is, and type the following:

```bash
 mkdir cppnetlib-build && cd cppnetlib-build
 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-std=c++11 \
 -stdlib=libc++ -arch x86_64 -Wl,-headerpad_max_install_names" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \ 
 -DCMAKE_C_FLAGS="-Wl,-headerpad_max_install_names -arch x86_64" -DCMAKE_INSTALL_PREFIX=/opt/local/ \ 
 -DCMAKE_EXE_LINKER_FLAGS=-headerpad_max_install_names -DCPP-NETLIB_BUILD_SHARED_LIBS=ON -DBUILD_SHARED_LIBS=ON \
 -DCMAKE_SHARED_LINKER_FLAGS=-headerpad_max_install_names -DCMAKE_MACOSX_RPATH=ON -DCMAKE_OSX_ARCHITECTURES=x86_64 \
 ../cpp-netlib-0.11.2-final
 make -j0 test && sudo make install
```

* Note: In Sierra, it is possible the mime-roundtrip test fails with a SIGSEGV while using boost 1.59. If this happens you need to install boost 1.60+ (instructions on doing so will require creating a custom port; you can find instructions to do this and the required files in the macports website)

* Note: Make sure the path to configure is correct, here I am using the default folder to which asioc++ is extracted. 

5. Clone Performous from git as per the other build instructions

6. Run the provided build script
* Open a terminal and navigate to `performous/osx-tools/`, then type

```bash
 chmod +x ./performous-app-build.sh && ./performous-app-build.sh
```

   * Note: There might be messages stating you need to install further dependencies for the build process. Follow on-screen instructions here
   * Note: Make sure to edit the performous-app-build.sh script and change the MAKE_JOBS variable to a number suitable to your machine, as a rule-of-thumb, you should change it to the lesser of how many CPU cores or Gb of RAM your computer has. 

7. This should create a file '''Performous.dmg''' in that directory. Mount that and drag Performous to Applications to install.

8. You'll most likely need to visit the audio configuration first in the configure menu. The integrated webserver can also be configured. Also check the wiki for supported paths for songs.

9. Play and have fun!


### MacOS with Homebrew

Building with Homebrew is easier than with MacPorts. However, creation of a software bundle from a Homebrew-build fails, so you will need to run Performous from console.

```bash
brew install boost cmake ffmpeg help2man icu4c portaudio portmidi \
    opencv libepoxy librsvg libxml++3 sdl2
```

Then follow the usual Performous build instructions. No special flags are required and you can do make install without sudo.

### Windows

Building in Windows is a bit trickier since you cannot get the dependencies as easily as in other systems. You should download the Git version (see [[Developing]]) and look at the scripts in win32 folder. They attempt to automatically set up the build environment (download and compile dependencies). Native builds should work with MSYS2, but have not been tested by anyone recently (that we know of, test reports welcome). The commonly used method is cross-compiling from Linux via MXE.

### Cross compiling MXE

The dependencies can be cross-compiled for Windows from Debian/Ubuntu Linux (possibly others, too), using the MinGW32/MinGW54 cross compiler. Best results are currently achieved using the "M cross environment" aka. [http://mxe.cc/ MXE]:

Get MXE:

```bash
git clone -b master https://github.com/mxe/mxe.git
```

Set up the settings.mk file (adjust JOBS to your number of CPU cores):

```bash
JOBS := 4
MXE_TARGETS :=  i686-w64-mingw32.shared
LOCAL_PKG_LIST := gettext sdl2 boost portaudio ffmpeg portmidi pango \
   gdk-pixbuf librsvg libsigc++ libxml++ libepoxy
.DEFAULT local-pkg-list:
local-pkg-list: $(LOCAL_PKG_LIST)
```

Run `make` to build the cross-compiler and packages (you can speed up the process by doing `make -j3`, where 3 is the number of CPU cores you wish to use).

When building later, replace the plain cmake command with these commands:

```bash
MXE_PREFIX=/where/mxe/is/installed
MXE_TARGET=i686-w64-mingw32.shared
cmake .. -DPKG_CONFIG_EXECUTABLE=$MXE_PREFIX/usr/bin/$MXE_TARGET-pkg-config \
 -DCMAKE_TOOLCHAIN_FILE=$MXE_PREFIX/usr/$MXE_TARGET/share/cmake/mxe-conf.cmake \
 -DBoost_THREAD_LIBRARY_RELEASE=$MXE_PREFIX/usr/$MXE_TARGET/bin/libboost_thread_win32-mt.dll \
 -DENABLE_WEBCAM=OFF
```

## Building

### Obtain latest source code

#### Recommended way: Git

Use [[Git Help|Git]] to get the latest development version from our public repository:

```bash
git clone git://github.com/performous/performous.git
```

You'll need a Git client. The command line program is fine, but if you prefer graphical interface, you can install e.g. git-gui on Linux and [TortoiseGit](https://tortoisegit.org) on Windows.


#### Alternative way: Tarballs

If you don't want to bother with git, you can just [download the bleeding edge code directly](https://github.com/performous/performous/archive/master.tar.gz).

Tarballs of stable releases are also available for distributors but they are not recommended if you are building it for yourself. You can download the released sources from [here](https://github.com/performous/performous/releases).


### Build and install

```bash
cd performous      # Where you downloaded the sources
mkdir build        # Make the build in a separate folder that you can easily clean up
cd build
cmake ..           # Prepare build and check that all dependencies are in place
make -j8           # Compile everything (replace 8 with number of CPU cores)
sudo make install  # Install
performous         # Start the game
```

If you get errors (especially if they are from the cmake command), you are probably missing some libraries required by the game. Install the needed libraries (development versions, e.g. libboost-dev) and retry the failing step until all dependencies are sorted out.

Optionally, if you want to change installation path or do other compile-time configuration, you can use ccmake command line tool in build directory, after running cmake:

```bash
ccmake .
```

There are also graphical interfaces for CMake, e.g. `cmake-qt-gui` on Ubuntu. Running cmake or ccmake is not required with GUI tools.

Before installing do `make install`, modify `CMAKE_INSTALL_PREFIX` if you don't want it installed in `/usr/local/`.


## How to Write a Good Issue

Please keep in mind that the GitHub issue tracker is not intended as a general support forum, but for reporting bugs and feature requests.
For end-user related support questions, please refer to one of the following:

- Discord Channel General: https://discord.gg/NS3m3ad

### Title

The title must be short and descriptive. (~60 characters)

### Description

- Respect the issue template as much as possible. [template](.github/ISSUE_TEMPLATE.md)
- Explain the conditions which led you to write this issue: the context.
- The context should lead to something, an idea or a problem that you’re facing.
- Remain clear and concise.
- Format your messages to help the reader focus on what matters and understand the structure of your message, use [Markdown syntax](https://help.github.com/articles/github-flavored-markdown)


## How to Write a Good Pull Request

### Title

The title must be short and descriptive. (~60 characters)

### Description

- Respect the pull request template as much as possible. [template](.github/PULL_REQUEST_TEMPLATE.md)
- Explain the conditions which led you to write this PR: the context.
- The context should lead to something, an idea or a problem that you’re facing.
- Remain clear and concise.
- Format your messages to help the reader focus on what matters and understand the structure of your message, use [Markdown syntax](https://help.github.com/articles/github-flavored-markdown)

### Content

- Make it small.
- Do only one thing.
- Write useful descriptions and titles.
- Avoid re-formatting.
- Make sure the code builds.
- Make sure all tests pass.
- Add tests.
- Address review comments in terms of additional commits.
- Do not amend/squash existing ones unless the PR is trivial.
- If a PR involves changes to third-party dependencies, the commits pertaining to the vendor folder and the manifest/lock file(s) should be committed separated.


Read [10 tips for better pull requests](http://blog.ploeh.dk/2015/01/15/10-tips-for-better-pull-requests/).
