if (NOT USE_SELF_BUILT_AUBIO)
    find_package(PkgConfig REQUIRED)
    if(PKG_CONFIG_FOUND)
        # The version we look for is legacy, may need some fine tunning
        pkg_check_modules(AUBIO IMPORTED_TARGET aubio>=0.4.9)
            if(AUBIO_FOUND)
                add_library(aubio ALIAS PkgConfig::AUBIO)
            endif()
    endif()
endif()

if (NOT AUBIO_FOUND)
    message("Building aubio from sources...")
    include(FetchContent)
    FetchContent_Declare(aubio-sources
      GIT_REPOSITORY https://github.com/performous/aubio.git
      GIT_TAG        14fec3da6749fbcc47b56648d7a38296eccd9499
      SOURCE_DIR aubio-src
    )
    FetchContent_MakeAvailable(aubio-sources)
endif()
