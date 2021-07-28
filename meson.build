# Project definition
project('performous',
        'cpp',
        version: '1.x',
        default_options: ['cpp_std=c++17','buildtype=debugoptimized'],
        meson_version: '>=0.53.0',
        license: 'GPL-2.0-or-later')

# Print project summary. Limited to strict minimum for now
summary({'Use Webcam': get_option('usewebcam'),
         'Use Midi Drum': get_option('usemididrum'),
         'Use Web Server': get_option('usewebserver'),
         'Compilation warnings as errors': get_option('warningaserror')
        }, section: 'Build Options')

# Add git describe output to version number.
# This does not appear work on semaphore, maybe because of stdout.
# If it fails, falls back to the project version defined above, without git reference.
version = meson.project_version()
git = find_program('git', required: true)
if git.found()
  gitcmd = run_command(git, 'describe')
  if gitcmd.returncode() == 0
    version = meson.project_version() + ' (git: ' + gitcmd.stdout().strip() + ')'
  endif
endif

# Configuration
conf = configuration_data()
conf.set('CMAKE_PROJECT_NAME', 'Performous')
conf.set('PROJECT_VERSION', version)
conf.set('LOCALE_DIR', 'share/locale')
conf.set('SHARE_INSTALL', 'share/games/performous')

# Convert options to project arguments
if get_option('usemididrum')
  add_project_arguments('-DUSE_PORTMIDI', language: 'cpp')
endif
if get_option('usewebcam')
  add_project_arguments('-DUSE_OPENCV', language: 'cpp')
endif
if get_option('usewebserver')
  add_project_arguments('-DUSE_WEBSERVER', language: 'cpp')
endif

# Compilers flags.  Some warnings really should be enabled (eg narrowing, overflow etc) but some legacy code
# has too many of those for the output to be useful.
# LTO is used if it is available on GCC (10+), but not on clang as it does not seem to be linking fine
compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'gcc'
  compiler_args = compiler.get_supported_arguments(
    ['-Wall','-Wextra','-pedantic','-Wc++20-compat',
    '-Wno-deprecated-declarations',
    '-Wno-mismatched-tags',
    '-Wno-narrowing',
    '-Wno-overflow',
    '-flto'])
    # Treat all warnings as errors if requested
    # Currently only for the gcc build, as clang returns more warnings that still need fixing
    if get_option('warningaserror')
      compiler_args += ['-Werror']
    endif
else
  compiler_args = compiler.get_supported_arguments(
    ['-Wall','-Wextra','-pedantic','-Wc++20-compat',
    '-Wno-c++11-narrowing',
    '-Wno-implicit-int-float-conversion',
    '-Wno-inconsistent-missing-override',
    '-Wno-deprecated-declarations',
    '-Wno-mismatched-tags'])
endif

# Link with stdc++fs if it is supplied by the compiler (gcc v8 or older, clang v10 or older)
# This is currently necessary on semaphore CI, which is running gcc8 max.
# Has_Link_Argument is only supported in meson 0.46+
if compiler.has_link_argument('-lstdc++fs')
  add_project_link_arguments(['-lstdc++fs'], language: 'cpp')
endif

# Dependencies
nulldependency = dependency('', required: false)

libxmlpp = dependency('libxml++-3.0', required: false)
if libxmlpp.found()
  conf.set('LIBXMLPP_VERSION_3_0', 1)
  conf.set('LIBXMLPP_VERSION_2_6', 0)
else
  libxmlpp = dependency('libxml++-2.6', required: true)
  conf.set('LIBXMLPP_VERSION_2_6', 1)
  conf.set('LIBXMLPP_VERSION_3_0', 0)
  warning('libxml++-2.6 is deprecated.  Use a more recent version if possible.')
endif

# OpenCV may be referred to as opencv or opencv4
opencv = nulldependency
if get_option('usewebcam')
  opencv = dependency('opencv', required: false)
  if not opencv.found()
    opencv = dependency('opencv4', required: true)
  endif
endif

# Portmidi is maybe not found in pkg-config, use find library as an alternative,
# with both names libportmidi (on ubuntu) and portmidi (on OSX)
portmidi = nulldependency
if get_option('usemididrum')
  portmidi = dependency('portmidi', required: false)
  if not portmidi.found()
    portmidi = compiler.find_library('libportmidi', required: false)
    if not portmidi.found()
      portmidi = compiler.find_library('portmidi', required: true)
    endif
  endif
endif

# Note: cpprest may not be in pkg-config
cpprest = nulldependency
libssl = nulldependency
libcrypto = nulldependency
if get_option('usewebserver')
  libssl = dependency('libssl', required: true)
  libcrypto = dependency('libcrypto', required: true)
  cpprest = dependency('cpprest', required: false)
  if not cpprest.found()
    cpprest = compiler.find_library('libcpprest', required: false)
    if not cpprest.found()
      cpprest = compiler.find_library('cpprest', required: false)
      if not cpprest.found()
        cpprest = dependency('cpprestsdk', required: true, method: 'cmake', modules: 'cpprestsdk::cpprest') # Macports
      endif
    endif
  endif
endif

boostmodules = ['iostreams','locale','program_options','system']
if not compiler.has_header('filesystem')
  boostmodules += ['filesystem'] # Boost::filesystem is used on systems where the standard filesystem is not available
  conf.set('USE_BOOST_FS', '1')
endif

# Write configuration file
configure_file(input: 'game/config.hh.in', output: 'config.hh', configuration: conf)

dependencies = [
  dependency('pangocairo'),
  dependency('sdl2'),
  dependency('aubio'),
  dependency('boost', modules: boostmodules),
  dependency('epoxy'),
  dependency('icu-uc'),
  dependency('icu-i18n'),
  dependency('icu-io'),
  dependency('libpng'),
  dependency('libjpeg'),
  dependency('librsvg-2.0'),
  dependency('libavcodec'),
  dependency('libavformat'),
  dependency('libavutil'),
  dependency('libswscale'),
  dependency('libswresample'),
  dependency('portaudio-2.0'),
  dependency('fontconfig'),
  dependency('glm'),
  libssl,
  libcrypto,
  libxmlpp,
  opencv,
  portmidi,
  cpprest]

executable('performous', [
  '3rdparty/ced/compact_enc_det/compact_enc_det.cc',
  '3rdparty/ced/compact_enc_det/compact_enc_det_hint_code.cc',
  '3rdparty/ced/util/encodings/encodings.cc',
  '3rdparty/ced/util/languages/languages.cc',
  'game/3dobject.cc',
  'game/audio.cc',
  'game/backgrounds.cc',
  'game/cache.cc',
  'game/color.cc',
  'game/configuration.cc',
  'game/controllers-joystick.cc',
  'game/controllers-keyboard.cc',
  'game/controllers-midi.cc',
  'game/controllers.cc',
  'game/dancegraph.cc',
  'game/database.cc',
  'game/dialog.cc',
  'game/engine.cc',
  'game/execname.cc',
  'game/ffmpeg.cc',
  'game/fs.cc',
  'game/game.cc',
  'game/glshader.cc',
  'game/glutil.cc',
  'game/guitargraph.cc',
  'game/hiscore.cc',
  'game/image.cc',
  'game/instrumentgraph.cc',
  'game/layout_singer.cc',
  'game/log.cc',
  'game/main.cc',
  'game/menu.cc',
  'game/midifile.cc',
  'game/musicalscale.cc',
  'game/notegraph.cc',
  'game/notes.cc',
  'game/opengl_text.cc',
  'game/pitch.cc',
  'game/platform.cc',
  'game/player.cc',
  'game/players.cc',
  'game/playlist.cc',
  'game/progressbar.cc',
  'game/requesthandler.cc',
  'game/screen_audiodevices.cc',
  'game/screen_intro.cc',
  'game/screen_paths.cc',
  'game/screen_players.cc',
  'game/screen_playlist.cc',
  'game/screen_practice.cc',
  'game/screen_sing.cc',
  'game/screen_songs.cc',
  'game/song.cc',
  'game/songitems.cc',
  'game/songparser-ini.cc',
  'game/songparser-mid.cc',
  'game/songparser-sm.cc',
  'game/songparser-txt.cc',
  'game/songparser-xml.cc',
  'game/songparser.cc',
  'game/songs.cc',
  'game/svg.cc',
  'game/texture.cc',
  'game/theme.cc',
  'game/unicode.cc',
  'game/util.cc',
  'game/video.cc',
  'game/video_driver.cc',
  'game/webcam.cc',
  'game/webserver.cc'],
  dependencies: dependencies,
  include_directories: include_directories('3rdparty/ced'),
  cpp_args: compiler_args,
  install: true
)

# Installation of data files
datapaths = [
  'data/backgrounds',
  'data/config',
  'data/fonts',
  'data/shaders',
  'data/sounds',
  'data/themes',
  'data/xsl']

foreach dp: datapaths
  install_subdir( dp, install_dir: 'share/games/performous')
endforeach

if host_machine.system() == 'linux'
  install_data('data/performous.desktop', install_dir: 'share/applications')
  install_data('data/themes/default/icon.svg', rename: 'performous.svg', install_dir: 'share/pixmaps')
endif

if host_machine.system() == 'darwin'
  install_data('data/performous.desktop', install_dir: 'Resources/applications')
  install_data('data/themes/default/icon.svg', rename: 'performous.svg', install_dir: 'Resources/pixmaps')
endif
