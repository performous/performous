project('performous', 'cpp', default_options : ['cpp_std=c++17'])

conf = configuration_data()
conf.set('CMAKE_PROJECT_NAME', 'Performous')
conf.set('PROJECT_VERSION', 'git-meson')
conf.set('LOCALE_DIR', 'share/locale')
conf.set('SHARE_INSTALL', 'share/games/performous')

libxmlpp = dependency('libxml++-3.0', required : false)
if libxmlpp.found()
  conf.set('LIBXMLPP_VERSION', 3)
else
  libxmlpp = dependency('libxml++-2.6')
  conf.set('LIBXMLPP_VERSION', 2)
endif

opencv = dependency('opencv4', required : false)
if opencv.found()
  add_project_arguments('-DUSE_OPENCV', language : 'cpp')
else
  warning('OpenCV not found:    Webcam support DISABLED')
endif

portmidi = dependency('PortMidi', required : false, cmake_module_path : 'cmake/Modules/')
if portmidi.found()
  add_project_arguments('-DUSE_PORTMIDI', language: 'cpp')
else
  warning('PORTMIDI not found:  MIDI drum support DISABLED')
endif

# This could in principle work, but doesn't in practice, at least not on my system
# - cpprestsdk doesn't support pkg-config
# - FindCppRest.cmake shipped with Performous doesn't find it for some reason
# - The library's installed cpprestsdk-config.cmake is not in path
# - This build script doesn't add the required extra Boost components to build
cpprest = dependency('CppRest', required : false, cmake_module_path : 'cmake/Modules/')
if cpprest.found()
  add_project_arguments('-DUSE_WEBSERVER', language: 'cpp')
else
  warning('CppRestSdk not found:  WEBSERVER support DISABLED')
endif

configure_file(input : 'game/config.hh.in', output : 'config.hh', configuration : conf)

dependencies = [
  dependency('pangocairo'),
  dependency('sdl2'),
  dependency('boost', modules : [
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
  libxmlpp,
  opencv,
  portmidi,
  cpprest,
]
ced = static_library('ced', [
  'ced/compact_enc_det/compact_enc_det.cc',
  'ced/compact_enc_det/compact_enc_det_hint_code.cc',
  'ced/util/encodings/encodings.cc',
  'ced/util/languages/languages.cc',
],
  include_directories : include_directories('ced'),
  cpp_args : ['-Wno-narrowing', '-Wno-unused-variable'],
)
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
  'game/webserver.cc',
],
  dependencies : dependencies,
  include_directories : include_directories('ced'),
  link_with: ced,
)
