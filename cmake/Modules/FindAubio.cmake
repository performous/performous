
if (NOT USE_SELF_BUILT_AUBIO)
    find_package(PkgConfig REQUIRED)
    if(PKG_CONFIG_FOUND)
        # The version we look for is legacy, may need some fine tunning
        pkg_check_modules(AUBIO IMPORTED_TARGET aubio>=0.4.9)
    endif()
endif()

if (NOT AUBIO_FOUND)

    message("Building aubio from sources...")
    include(FetchContent)
    FetchContent_Declare(aubio-sources
      GIT_REPOSITORY https://github.com/performous/aubio.git
      GIT_TAG        fcf3b14c6ee5e5a658a5683ac303e56c8ade2432
      SOURCE_DIR aubio-src
    )
    FetchContent_MakeAvailable(aubio-sources)

endif()
