INCLUDE(FindOpenGL)
INCLUDE(FindSDL)
INCLUDE(FindSDL_mixer)

IF (CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS (-g)
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

FIND_PATH(FREETYPE2_INCLUDE_DIR freetype/freetype.h
  ../include/freetype2
  /usr/local/include/freetype2
  /usr/include/freetype2
  )

ADD_LIBRARY(silkystrings-convenience
  Action.cpp
  chord.cpp
  GameView.cpp
  GLExtensionProxy.cpp
  Input.cpp
  Mesh.cpp
  MeshFactory.cpp
  midiEvent.cpp
  midiFileParser.cpp
  midiStream.cpp
  SoftwareElementDataBuffer.cpp
  SoftwareVertexDataBuffer.cpp
  Sound.cpp
  Reaction.cpp
  TextRenderer.cpp
  Texture2D.cpp
  VertexDataBufferManager.cpp
  VertexFormat.cpp
  WM.cpp
)

IF (NOT WIN32)
  SET (GLFW_EXTRA_LIBS Xxf86vm)
ENDIF (NOT WIN32)

TARGET_LINK_LIBRARIES(silkystrings-convenience
  glfw${GLFW_SUFFIX}
  ${OPENGL_LIBRARIES}
  freetype${FREETYPE_SUFFIX}
  ${GLFW_EXTRA_LIBS}
  ${SDL_LIBRARY}
  ${SDLMIXER_LIBRARY}
)

IF (CMAKE_COMPILER_IS_GNUCXX)
  SET_TARGET_PROPERTIES(silkystrings-convenience PROPERTIES COMPILE_FLAGS "-O2 -fno-inline-functions -Wall -Wextra -Wno-unused-parameter")
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

INCLUDE_DIRECTORIES(
  ${FREETYPE2_INCLUDE_DIR}
  ${GLFW_INCLUDE_DIR}
  ${BOOST_INCLUDE_DIR}
  ${FMODEX_INCLUDE_DIR}
  ${SDL_INCLUDE_DIR}
  ${SDLMIXER_INCLUDE_DIR}
)

LINK_DIRECTORIES(${BOOST_LIB_DIR})

INCLUDE(FindQt4)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

QT4_AUTOMOC(LauncherWindow.cpp)

ADD_EXECUTABLE(silkystrings-launcher LauncherWindow.cpp SongIterator.cpp)
TARGET_LINK_LIBRARIES(silkystrings-launcher ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} boost_filesystem${BOOST_SUFFIX})

ADD_EXECUTABLE(silkystrings silkystrings.cpp)
TARGET_LINK_LIBRARIES(silkystrings silkystrings-convenience)

