#Project definition
project('performous',
        'cpp',
        version: '1.x',
        default_options : ['cpp_std=c++17','buildtype=debugoptimized'],
        meson_version: '>=0.53.0',
        license : 'GPL-2.0-or-later')

compact_enc_det = subproject('ced')
nlohmann_json = subproject('nlohmann_json')

compact_enc_det_dep = compact_enc_det.get_variable('compact_enc_det_dep') 
nlohmann_json_dep = dependency('nlohmann_json', fallback : [nlohmann_json, nlohmann_json_dep])

#Print project summary.  Limited to strict minimum for now
summary({'Use Open CV': get_option('usewebcam'),
         'Use Port Midi': get_option('usemididrum'),
         'Use Web Server': get_option('usewebserver')
        }, section: 'Build Options')

#Add git describe output to version number.
#This does not appear work on semaphore, maybe because of stdout.
#If it fails, falls back to the project version defined above, without git reference.
version = meson.project_version()
git = find_program('git', required: true)
if git.found()
  gitcmd = run_command(git, 'describe')
  if gitcmd.returncode() == 0
    version = meson.project_version()+' (git: '+gitcmd.stdout().strip() +')'
  endif
endif

#Configuration
conf = configuration_data()
conf.set('CMAKE_PROJECT_NAME', 'Performous')
conf.set('PROJECT_VERSION', version)
conf.set('LOCALE_DIR', 'share/locale')
conf.set('SHARE_INSTALL', 'share/games/performous')

#Compilers flags.  Some warnings really should be enabled (eg narrowing, overflow etc) but some legacy code
#has too many of those for the output to be useful.
#LTO is used if it is available on GCC (10+), but not on clang as it does not seem to be linking fine
compiler=meson.get_compiler('cpp')
if compiler.get_id() == 'gcc'
  compiler_args=compiler.get_supported_arguments(
    ['-Wall','-Wextra','-pedantic','-Wc++20-compat',
    '-Wno-deprecated-declarations',
    '-Wno-mismatched-tags',
    '-Wno-narrowing',
    '-Wno-overflow',
    '-flto'])
  #Treat all warnings as errors if requested
  if get_option('stoponwarning')
    compiler_args+=['-Werror']
  endif
else
  compiler_args=compiler.get_supported_arguments(
    ['-Wall','-Wextra','-pedantic','-Wc++20-compat',
    '-Wno-c++11-narrowing',
    '-Wno-implicit-int-float-conversion',
    '-Wno-inconsistent-missing-override',
    '-Wno-deprecated-declarations',
    '-Wno-mismatched-tags'])
endif

#Link with stdc++fs if it is supplied by the compiler (gcc v8 or older, clang v10 or older)
#This is currently necessary on semaphore CI, which is running gcc8 max.
#Has_Link_Argument is only supported in meson 0.46+
if compiler.has_link_argument('-lstdc++fs')
  add_project_link_arguments(['-lstdc++fs'], language: 'cpp')
endif

#Dependencies
libxmlpp = dependency('libxml++-3.0', required : false)
if libxmlpp.found()
  conf.set('LIBXMLPP_VERSION_3_0', 1)
  conf.set('LIBXMLPP_VERSION_2_6', 0)
else
  libxmlpp = dependency('libxml++-2.6')
  conf.set('LIBXMLPP_VERSION_2_6', 1)
  conf.set('LIBXMLPP_VERSION_3_0', 0)
endif

#OpenCV may be referred to as opencv or opencv4
if get_option('usewebcam')
  message('OpenCV build requested')
  opencv = dependency('opencv', required: false)
  if opencv.found()
      add_project_arguments('-DUSE_OPENCV', language : 'cpp')
  else
    opencv = dependency('opencv4', method : 'pkg-config', required: false)
    if opencv.found()
      add_project_arguments('-DUSE_OPENCV', language : 'cpp')
    else
      error('OpenCV not found but webcam build requested')
    endif
  endif
else
  opencv=dependency('',required: false)
endif

#Portmidi is maybe not found in pkg-config, use find library as an alternative
if get_option('usemididrum')
  message('Portmidi build requested')
  portmidi = dependency('portmidi', required: false)
  if portmidi.found()
    add_project_arguments('-DUSE_PORTMIDI', language: 'cpp')
  else
    portmidi = compiler.find_library('libportmidi', required: false)
    if portmidi.found()
      add_project_arguments('-DUSE_PORTMIDI', language: 'cpp')
    else
      error('PORTMIDI not found but MIDI drum support build requested')
    endif
  endif
else
  portmidi=dependency('',required: false)
endif

#Cpprest may not be in pkg-config
#Also, libssl and libcrypto are only needed when webserver support is requested
if get_option('usewebserver')
  message('Webserver build requested')
  cpprest=dependency('cpprest', required : false)
  if cpprest.found()
    add_project_arguments('-DUSE_WEBSERVER', language: 'cpp')
  else
    cpprest=compiler.find_library('libcpprest', required:false)
    if cpprest.found()
      add_project_arguments('-DUSE_WEBSERVER', language: 'cpp')
    else
      error('CppRestSdk not found but webserver build requested')
    endif
  endif
else
  cpprest=dependency('',required: false)
endif

#Write configuration file
configure_file(input : 'game/config.hh.in', output : 'config.hh', configuration : conf)

dependencies = [
  dependency('pangocairo'),
  dependency('sdl2'),
  dependency('aubio'),
  dependency('boost', modules : [ # Meson treats boost specially: no pkg-config supported
    'filesystem',
    'iostreams',
    'locale',
    'program_options',
    'system'
  ]),
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
  dependency('libssl', required:get_option('usewebserver')), #Only needed with webserver
  dependency('libcrypto', required:get_option('usewebserver')), #Idem
  libxmlpp,
  opencv,
  portmidi,
  cpprest
]

executable('performous', [
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
  'game/webserver.cc'
],
  dependencies : dependencies,
  cpp_args : compiler_args,
  install: true
)

#Installation of data files
datapaths = [
  'data/backgrounds',
  'data/config',
  'data/fonts',
  'data/shaders',
  'data/sounds',
  'data/themes',
  'data/xsl'
]

foreach dp: datapaths
  install_subdir(dp, install_dir: 'share/games/performous')
endforeach

# TBD: desktop file, pixmap files, man etc. see cmake of data and docs dir
# help2man = find_program('help2man', required: false)
# if help2man.found()
#   help2man_opts = [
#     '--no-info',
#     '--section=1',
# Maybe also add a target to build a debian pkg ...
