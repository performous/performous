
if (NOT USE_SELF_BUILT_AUBIO)
    find_package(PkgConfig REQUIRED)
    # The version we look for is legacy, may need some fine tunning
    pkg_check_modules(AUBIO IMPORTED_TARGET aubio>=0.4.9)
endif()

if (NOT AUBIO_FOUND)

    message("Building aubio from sources...")
    include(FetchContent)
    FetchContent_Declare(aubio-sources
      GIT_REPOSITORY https://github.com/performous/aubio.git
      GIT_TAG        2bf3653dc6d9a10ba86714101ca1c916a963e93d
      SOURCE_DIR aubio-src
    )
    FetchContent_MakeAvailable(aubio-sources)

endif()