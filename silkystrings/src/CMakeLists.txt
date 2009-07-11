cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Libraries

#include_directories(${CMAKE_SOURCE_DIR}/libs/libda/include)

find_package(Boost 1.34 REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

# Find all the libs that don't require extra parameters
foreach(lib SDL OpenGL GLFW Freetype SDL_mixer)
	find_package(${lib} REQUIRED)
	include_directories(${${lib}_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, enabling warnings")
	# -pedantic cannot be used because ffmpeg headers are b0rked
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_library(silkystrings-convenience
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

target_link_libraries(silkystrings-convenience ${LIBS})

add_executable(silkystrings silkystrings.cpp)
target_link_libraries(silkystrings silkystrings-convenience)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
QT4_AUTOMOC(LauncherWindow.cpp)
include_directories(${CMAKE_CURRENT_BINARY_DIR})  # For the MOC file
add_executable(silkystrings-launcher LauncherWindow.cpp SongIterator.cpp)
target_link_libraries(silkystrings-launcher ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${Boost_LIBRARIES})

install(TARGETS silkystrings silkystrings-launcher DESTINATION bin)

