# FindUnwind.cmake — shim to prefer the system libunwind DSO over the
# getdeps-installed static libunwind.a.
#
# The getdeps static libunwind.a references `uncompress` (from zlib) but at
# link time zlib appears before libunwind in the command, so the symbol is
# "already consumed" and not re-resolved.  The system libunwind.so is a DSO
# that stays in the linker's view for all archives that follow it.

find_path(Unwind_INCLUDE_DIR
    NAMES libunwind.h
    PATHS /usr/include /usr/local/include
    HINTS ENV UNWIND_ROOT
)

find_library(Unwind_LIBRARY
    NAMES unwind
    PATHS /usr/lib /usr/lib64
          /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu
    HINTS ENV UNWIND_ROOT
)

# libunwind provides unwind.h which may contain the version
set(Unwind_VERSION "")
if(Unwind_INCLUDE_DIR AND EXISTS "${Unwind_INCLUDE_DIR}/unwind.h")
    # Treat as satisfying any version requirement
    set(Unwind_VERSION "1.0")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Unwind
    FOUND_VAR Unwind_FOUND
    REQUIRED_VARS Unwind_LIBRARY Unwind_INCLUDE_DIR
)

if(Unwind_FOUND)
    # Folly's installed cmake configs reference the namespaced target
    # `unwind::unwind`.  Provide both names so any downstream link list works.
    if(NOT TARGET unwind::unwind)
        add_library(unwind::unwind UNKNOWN IMPORTED)
        set_target_properties(unwind::unwind PROPERTIES
            IMPORTED_LOCATION "${Unwind_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Unwind_INCLUDE_DIR}"
        )
    endif()
    if(NOT TARGET unwind)
        add_library(unwind INTERFACE IMPORTED)
        set_target_properties(unwind PROPERTIES
            INTERFACE_LINK_LIBRARIES unwind::unwind
        )
    endif()
endif()

mark_as_advanced(Unwind_INCLUDE_DIR Unwind_LIBRARY)
