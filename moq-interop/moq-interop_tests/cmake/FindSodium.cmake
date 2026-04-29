# FindSodium.cmake — shim for getdeps-installed libsodium
#
# libsodium is built by getdeps (Facebook's build tool) but does not ship
# a CMake Config file — only a pkg-config .pc.  fizz-config.cmake calls
# find_dependency(Sodium), which routes here via CMAKE_MODULE_PATH.
#
# This module searches CMAKE_PREFIX_PATH (populated from getdeps env) for the
# headers and library, then creates an imported target Sodium::sodium compatible
# with the one fizz expects.

find_path(Sodium_INCLUDE_DIR
    NAMES sodium.h
    PATH_SUFFIXES include
)

find_library(Sodium_LIBRARY
    NAMES sodium
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sodium
    REQUIRED_VARS Sodium_LIBRARY Sodium_INCLUDE_DIR
)

if(Sodium_FOUND AND NOT TARGET Sodium::sodium)
    add_library(Sodium::sodium UNKNOWN IMPORTED)
    set_target_properties(Sodium::sodium PROPERTIES
        IMPORTED_LOCATION "${Sodium_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Sodium_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(Sodium_INCLUDE_DIR Sodium_LIBRARY)
