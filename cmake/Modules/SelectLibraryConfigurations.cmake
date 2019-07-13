# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of Contributors
#   may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#.rst:
# SelectLibraryConfigurations
# ---------------------------
#
#
#
# select_library_configurations( basename )
#
# This macro takes a library base name as an argument, and will choose
# good values for basename_LIBRARY, basename_LIBRARIES,
# basename_LIBRARY_DEBUG, and basename_LIBRARY_RELEASE depending on what
# has been found and set.  If only basename_LIBRARY_RELEASE is defined,
# basename_LIBRARY will be set to the release value, and
# basename_LIBRARY_DEBUG will be set to basename_LIBRARY_DEBUG-NOTFOUND.
# If only basename_LIBRARY_DEBUG is defined, then basename_LIBRARY will
# take the debug value, and basename_LIBRARY_RELEASE will be set to
# basename_LIBRARY_RELEASE-NOTFOUND.
#
# If the generator supports configuration types, then basename_LIBRARY
# and basename_LIBRARIES will be set with debug and optimized flags
# specifying the library to be used for the given configuration.  If no
# build type has been set or the generator in use does not support
# configuration types, then basename_LIBRARY and basename_LIBRARIES will
# take only the release value, or the debug value if the release one is
# not set.

# This macro was adapted from the FindQt4 CMake module and is maintained by Will
# Dicharry <wdicharry@stellarscience.com>.

macro( select_library_configurations basename )
    if(NOT ${basename}_LIBRARY_RELEASE)
        set(${basename}_LIBRARY_RELEASE "${basename}_LIBRARY_RELEASE-NOTFOUND" CACHE FILEPATH "Path to a library.")
    endif()
    if(NOT ${basename}_LIBRARY_DEBUG)
        set(${basename}_LIBRARY_DEBUG "${basename}_LIBRARY_DEBUG-NOTFOUND" CACHE FILEPATH "Path to a library.")
    endif()

    if( ${basename}_LIBRARY_DEBUG AND ${basename}_LIBRARY_RELEASE AND
           NOT ${basename}_LIBRARY_DEBUG STREQUAL ${basename}_LIBRARY_RELEASE AND
           ( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE ) )
        # if the generator supports configuration types or CMAKE_BUILD_TYPE
        # is set, then set optimized and debug options.
        set( ${basename}_LIBRARY "" )
        foreach( _libname IN LISTS ${basename}_LIBRARY_RELEASE )
            list( APPEND ${basename}_LIBRARY optimized "${_libname}" )
        endforeach()
        foreach( _libname IN LISTS ${basename}_LIBRARY_DEBUG )
            list( APPEND ${basename}_LIBRARY debug "${_libname}" )
        endforeach()
    elseif( ${basename}_LIBRARY_RELEASE )
        set( ${basename}_LIBRARY ${${basename}_LIBRARY_RELEASE} )
    elseif( ${basename}_LIBRARY_DEBUG )
        set( ${basename}_LIBRARY ${${basename}_LIBRARY_DEBUG} )
    else()
        set( ${basename}_LIBRARY "${basename}_LIBRARY-NOTFOUND")
    endif()

    set( ${basename}_LIBRARIES "${${basename}_LIBRARY}" )

    if( ${basename}_LIBRARY )
        set( ${basename}_FOUND TRUE )
    endif()

    mark_as_advanced( ${basename}_LIBRARY_RELEASE
        ${basename}_LIBRARY_DEBUG
    )
endmacro()
