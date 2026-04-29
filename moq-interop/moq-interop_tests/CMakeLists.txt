# =============================================================================
# moq-interop_tests — CMake build
#
# Single, unified build that works for Docker (primary) and local Linux/macOS
# builds. All third-party dependencies are pulled in transitively via
# find_package(moxygen CONFIG REQUIRED), which in turn calls find_dependency()
# for folly, fizz, mvfst, proxygen, and wangle.
#
# Prerequisites
#   - moxygen and its dependencies built and installed via
#     `python3 build/fbcode_builder/getdeps.py build moxygen` (see scripts/build.sh)
#   - CMAKE_PREFIX_PATH set to the getdeps install root (the build.sh wrapper
#     does this automatically by `eval`-ing the getdeps environment)
#
# Cache variables
#   SANITIZE  Empty (default), "address", "thread", or "undefined"
# =============================================================================

cmake_minimum_required(VERSION 3.20)
project(moq_interop_tests LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# ─── Sanitizer support ──────────────────────────────────────────────────────
set(SANITIZE "" CACHE STRING "Sanitizer to enable: address, thread, undefined")
if(SANITIZE)
    message(STATUS "Sanitizer enabled: ${SANITIZE}")
    add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer -g -O1)
    add_link_options(-fsanitize=${SANITIZE})
endif()

# ─── Dependency discovery ───────────────────────────────────────────────────
# moxygen-config.cmake calls find_dependency(folly|wangle|mvfst|proxygen|fizz),
# which in turn calls find_dependency(Sodium) from fizz-config.cmake.
# libsodium is built by getdeps but does not ship a CMake config file —
# only a pkg-config .pc.  We supply a FindSodium.cmake shim that locates the
# getdeps-installed libsodium via CMAKE_PREFIX_PATH so the transitive chain
# resolves cleanly.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Linux: folly's installed cmake config bakes the target `unwind::unwind`
# into INTERFACE_LINK_LIBRARIES but does NOT issue a find_dependency for it,
# so we have to call find_package(Unwind) ourselves before linking against
# any folly target.  Our cmake/FindUnwind.cmake shim creates the namespaced
# target backed by the system libunwind DSO.
if(UNIX AND NOT APPLE)
    find_package(Unwind REQUIRED)
endif()

find_package(moxygen CONFIG REQUIRED)

# ─── Sources ────────────────────────────────────────────────────────────────
set(SOURCES
    main.cpp
    test_suite.cpp
    test_registry.cpp
    base/base_test.cpp
    base/moqt_interface.cpp
    publisher_tests/publish_test.cpp
    publisher_tests/publish_namespace_test.cpp
    publisher_tests/publish_namespace_done_test.cpp
    publisher_tests/goaway_test.cpp
    publisher_tests/max_request_id_test.cpp
    subscriber_tests/fetch_test.cpp
    subscriber_tests/subscribe_namespace_test.cpp
    subscriber_tests/subscribe_test.cpp
    subscriber_tests/subscribe_error_test.cpp
    subscriber_tests/subscribe_update_test.cpp
    subscriber_tests/subscribe_update_error_test.cpp
    subscriber_tests/track_status_test.cpp
    moxygen_adapter/moxygen_interface.cpp
    moxygen_adapter/moxygen_mocks.cpp
    moxygen_adapter/moxygen_fixture.cpp
)

# Linux: stub for rust_demangle_callback. folly is built with
# FOLLY_HAVE_RUST_DEMANGLE=1 by getdeps when rustc is detected, but the
# Docker builder image does not ship librustc-demangle-capi.  This stub
# satisfies the linker; Rust frames in folly stack traces just stay mangled.
if(UNIX AND NOT APPLE)
    list(APPEND SOURCES stub_demangle.c)
endif()

add_executable(interop_tests ${SOURCES})

target_include_directories(interop_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The moxygen adapter sources are always compiled; define the macro so
# test_suite.cpp creates and attaches the fixture.
target_compile_definitions(interop_tests PRIVATE WITH_MOXYGEN_ADAPTER)

# ─── Linkage ────────────────────────────────────────────────────────────────
# Linking against these moxygen targets transitively pulls in everything else
# (folly, fizz, mvfst, proxygen, wangle, glog, gflags, fmt, …) via the
# EXPORTED_DEPS declared in moxygen's CMakeLists.txt.
# main.cpp uses folly::Init and folly::EventBaseThread directly, so those
# folly targets must be listed explicitly.
target_link_libraries(interop_tests PRIVATE
    moxygen::moxygen_moqclient
    moxygen::moxygen_moq_relay_session
    moxygen::moxygen_moq_consumers
    moxygen::moxygen_moq_framer
    moxygen::moxygen_moq
    Folly::folly
)

# Linux: libiberty provides cplus_demangle_v3_callback referenced by folly's
# static archive.  libiberty-dev is installed in the Docker builder image;
# on local Ubuntu/Debian: `apt install libiberty-dev`.
# lzma / libunwind: getdeps libunwind.a is placed after liblzma.a in the
# generated link command; when the system liblzma.so.5 is a DSO it must be
# explicitly present on the command line when the archive that needs it is
# processed.  --copy-dt-needed-entries tells ld to follow DT_NEEDED tags of
# shared objects already on the command line, resolving the ABI symbol
# lzma_stream_buffer_decode without requiring duplicate -llzma placement.
if(UNIX AND NOT APPLE)
    target_link_libraries(interop_tests PRIVATE iberty z)
    target_link_options(interop_tests PRIVATE
        -Wl,--copy-dt-needed-entries
    )
endif()

set_target_properties(interop_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "moq-interop_tests configured")
message(STATUS "  build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  sanitizer  : ${SANITIZE}")
message(STATUS "  output     : ${CMAKE_BINARY_DIR}/bin/interop_tests")
