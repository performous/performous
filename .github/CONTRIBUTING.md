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
   libopencv-dev libportmidi-dev libcpprest-dev nlohmann-json3-dev libfmt-dev \
   libwebp-dev
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
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
sudo dnf install https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
```

#### Fedora 42 and earlier
```bash
sudo dnf install git cmake gcc-c++ gettext cairo-devel librsvg2-devel libsigc++20-devel \
   glibmm24-devel libxml++-devel boost-devel SDL2-devel libepoxy-devel ffmpeg-devel \
   portaudio-devel help2man redhat-lsb opencv-devel portmidi-devel libjpeg-turbo-devel \
   pango-devel jsoncpp-devel fmt-devel libwebp-devel
```

#### Fedora 43 and later
```bash
sudo dnf install git cmake gcc-c++ gettext cairo-devel librsvg2-devel libsigc++20-devel \
glibmm24-devel libxml++-devel boost-devel SDL2-devel libepoxy-devel ffmpeg-devel \
portaudio-devel help2man lsb-release opencv-devel portmidi-devel libjpeg-turbo-devel \
pango-devel jsoncpp-devel glm-devel openblas-devel fftw-devel cpprest-devel \
libwebp-devel fmt-devel
```
(You may need to include `--allowerasing` for this to complete successfully)

#### Unit Tests
If you also plan to run unit tests, further packages are required.
(This is **not** needed just to play Performous)
```bash
sudo dnf install gtest-devel gmock-devel
```

### MacOS

These instructions cover both a quick dev build (compile and run from the build tree) and building a distributable, relinked `.app`/`.dmg`. MacPorts is the only package manager tested and supported — it's what CI (`.github/workflows/macports.yml`) uses to build and test every push, on both Apple Silicon and Intel. The deployment target is macOS 15.

#### 1. Prerequisites

* Xcode Command Line Tools: `xcode-select --install`
* Install [MacPorts](https://www.macports.org/install.php)

```bash
sudo port selfupdate
echo macosx_deployment_target 15.0 | sudo tee -a /opt/local/etc/macports/macports.conf
```

#### 2. Dependencies

```bash
sudo port install boost cairo cmake cpprestsdk dylibbundler ffmpeg7 \
   fftw-3-single libfmt11 fontconfig freetype glm help2man icu libepoxy \
   librsvg libsdl2 libxmlxx5 nlohmann-json opencv4 openssl pango \
   portaudio portmidi
```

Some of these (e.g. `ffmpeg7`) have no prebuilt archive for every macOS version and get compiled from source by MacPorts, which can pull in further source builds transitively. If one of those fails with errors like `fatal error: 'memory' file not found`/`'cstdint' file not found` for standard C/C++ headers, your Command Line Tools installation has stale leftover headers (a known MacPorts/CLT interaction — MacPorts prints its own warning about this during `clean`). In this case, just reinstall the CommandLineTools:

```bash
sudo rm -rf /Library/Developer/CommandLineTools
xcode-select --install
```

Then re-run the `port install` command above.

If you plan to use the [CMake presets](#quick-dev-build-cmake-presets) below for a plain dev build, also install a generator, like ninja: `sudo port install ninja`. For running unit tests, install gtest: `sudo port install gtest`.

#### 3. Clone Performous

Clone the repository as per the standard build instructions (see [Obtain latest source code](#obtain-latest-source-code) below).

#### 4. Quick dev build (CMake presets)

```bash
# Configure (only needed once, or after CMakeLists.txt/preset changes)
cmake --preset macos-x64-debug

# Build (re-run this after every code change)
cmake --build --preset macos-x64-debug && cmake --install build/macos-x64-debug
```

Swap `debug` for `release` or `debinfo` as needed. This produces a complete `Performous.app` under `build/macos-x64-debug-install/Performous.app`, which you can launch directly:

```bash
# Remember to launch the in-app executable to ensure you have access to the microphones
./build/macos-x64-debug-install/Performous.app/Contents/MacOS/Performous
```

This is sufficient for running and debugging locally, but it is not relinked for distribution. For a distributable bundle, use the bundler script below instead.

#### 5. Building a distributable .app/.dmg (macos-bundler.py)

`osx-utils/macos-bundler.py` is the script CI uses to produce the actual release `.app`/`.dmg`: it configures and builds Performous, relinks all dependent dylibs into the bundle via `dylibbundler`, and packages a `.dmg`.

MacPorts' stock `librsvg` port isn't built with enough Mach-O header padding for `dylibbundler`'s relinking step, which fails with `larger updated load commands do not fit`. CI works around this with a small local Portfile patch; do the same once, before building:

```bash
sudo mkdir -p /opt/custom_portfiles/graphics/
sudo cp -R $(dirname $(port file librsvg)) /opt/custom_portfiles/graphics/
sudo bash -c 'cat <<EOF >> /opt/custom_portfiles/graphics/librsvg/Portfile
configure.ldflags-append -Wl,-headerpad_max_install_names
EOF'
sudo bash -c 'cat <<EOF > /opt/local/etc/macports/sources.conf
file:///opt/custom_portfiles [nosync]
rsync://rsync.macports.org/macports/release/tarballs/ports.tar [default]
EOF'
sudo portindex /opt/custom_portfiles
sudo port -f uninstall librsvg
sudo port install librsvg
```

Then build:

```bash
cd osx-utils
python3 -m venv ./bundler-venv
source ./bundler-venv/bin/activate
pip3 install -r ./macos-bundler-requirements.txt
python3 ./macos-bundler.py
```

* `--debug` — build a `.app` for local debugging, skipping dylib relinking and `.dmg` creation (fast iteration, not for distribution).
* `--xcode-project` — generate an Xcode project for debugging in the IDE.

Run `python3 ./macos-bundler.py --help` for the full list of options.

You'll most likely need to visit the audio configuration first in the in-game configure menu. The integrated webserver can also be configured. Also check the wiki for supported paths for songs.

#### Homebrew

MacPorts remains the recommended, CI-verified package manager. Homebrew is community-supported on a best-effort basis for dev builds only.

```bash
brew install boost cairo cmake cpprestsdk dylibbundler ffmpeg fontconfig freetype \
   glm googletest help2man icu4c libepoxy librsvg libxml++3 nlohmann-json \
   opencv pango portaudio portmidi sdl2
```

(`cpprestsdk` is deprecated upstream — its repository is archived — but Homebrew still bottles it; it's only needed for webserver support.)

Then, from `osx-utils/`, run the bundler script in debug mode with `--prefer-homebrew --debug`. Building the dmg (not using `--debug`) will fail.

```bash
cd osx-utils
python3 -m venv ./bundler-venv
source ./bundler-venv/bin/activate
pip3 install -r ./macos-bundler-requirements.txt
python3 ./macos-bundler.py --prefer-homebrew --debug
```

One known Homebrew-specific issue this project works around: Homebrew's `fmt` package is often newer than this project's vendored `spdlog` expects, which used to cause `-Werror`/`-Wdeprecated-declarations` build failures. This is handled (`cmake/Modules/FindSpdlog.cmake` marks the vendored spdlog headers as system headers), so it shouldn't need any manual workaround. If you hit a similar deprecation error building against a very new Homebrew library, that's the class of issue to look for.

#### Known issues

* Arabic and other right-to-left text does not currently render correctly on macOS; this is suspected to be a font/fontconfig configuration issue rather than a rendering-engine bug (see `game/unicode.hh`).

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
