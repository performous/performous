
option(USE_SELF_BUILT_AUBIO "Use custom aubio local build instead of using system lib (if available)" FALSE)

if (NOT USE_SELF_BUILT_AUBIO)
    find_package(PkgConfig REQUIRED)
    # The version we look for is legacy, may need some fine tunning
    pkg_check_modules(AUBIO IMPORTED_TARGET aubio>=0.4.9)
endif()

if (NOT AUBIO_FOUND)

if (APPLE)
	set_property(GLOBAL PROPERTY BLA_VENDOR APPLE)
	set(ACCELERATE_COMMAND "--enable-accelerate")
	set(AUBIO_CFLAGS "${CMAKE_C_FLAGS} -iframeworkwithsysroot /System/Library/Frameworks/ -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -isysroot '${CMAKE_OSX_SYSROOT}/'")
else()
	set(ACCELERATE_COMMAND "--enable-atlas")
        set_property(GLOBAL PROPERTY BLA_VENDOR OpenBLAS)
	set(AUBIO_CFLAGS "${CMAKE_C_FLAGS}")
endif()

find_package(BLAS REQUIRED MODULE)
find_package(FFTW3 COMPONENTS single REQUIRED MODULE)

if(NOT FFTW3_FOUND OR NOT BLAS_FOUND)
	message(ERROR "fftw3 or BLAS required to build aubio, but they cannot be found.")
endif()

find_program (WAF NAMES waf PATHS ENV PATH)

if (NOT WAF)
    FILE(DOWNLOAD https://waf.io/waf-2.0.22 ${CMAKE_BINARY_DIR}/my_waf EXPECTED_HASH MD5=f2e5880ba4ecd06f7991181bdba1138b)
    set(WAF python3 ${CMAKE_BINARY_DIR}/my_waf)
endif()

message("Building aubio from sources... ${WAF}")
include(ExternalProject)
ExternalProject_Add(build-aubio-from-sources
        GIT_REPOSITORY https://git.aubio.org/aubio/aubio/
        GIT_TAG 0.4.9
        GIT_SHALLOW TRUE
        SOURCE_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio
        BINARY_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-build/
        INSTALL_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-install/
        UPDATE_COMMAND ""
	TMP_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-build/tmp/
	CONFIGURE_COMMAND ""
	BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/3rdparty/aubio-install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aubio${CMAKE_STATIC_LIBRARY_SUFFIX}
	                 ${CMAKE_BINARY_DIR}/3rdparty/aubio-install/include/aubio/aubio.h
	BUILD_COMMAND ${CMAKE_COMMAND} -E env CFLAGS=${AUBIO_CFLAGS}
                  CXX=${CMAKE_CXX_COMPILER}
                  CC=${CMAKE_C_COMPILER}
                  ${WAF} ${ACCELERATE_COMMAND} --disable-tests --notests --verbose
                  --enable-fftw3f --disable-sndfile --disable-avcodec --disable-double
                  --disable-samplerate --disable-docs --disable-wavread --disable-wavwrite
                  --disable-tests --notests --disable-examples --disable-apple-audio
                  --disable-fat --disable-jack --libdir=lib
                  --prefix=${CMAKE_BINARY_DIR}/3rdparty/aubio-install/
                  --top=${CMAKE_BINARY_DIR}/3rdparty/aubio/
                  --out=${CMAKE_BINARY_DIR}/3rdparty/aubio-build/ configure build install
	INSTALL_COMMAND ""
)

ExternalProject_Get_Property(build-aubio-from-sources INSTALL_DIR)
set (AUBIO_INSTALL_DIR ${INSTALL_DIR})
file(MAKE_DIRECTORY ${INSTALL_DIR}/include)

add_library(aubio STATIC IMPORTED)
set_target_properties(aubio PROPERTIES IMPORTED_LOCATION ${AUBIO_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aubio${CMAKE_STATIC_LIBRARY_SUFFIX})

set_target_properties(aubio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${AUBIO_INSTALL_DIR}/include/)

endif()
