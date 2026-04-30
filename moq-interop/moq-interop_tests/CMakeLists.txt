cmake_minimum_required(VERSION 3.16)
project(interop_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----- Adapter option --------------------------------------------------------
# ON  (default) - full build: moxygen_adapter + all subscriber tests
# OFF           - core build: base/, publisher_tests/, fetch + subscribe_namespace
#                 Still requires folly from the getdeps build.
option(WITH_MOXYGEN_ADAPTER
    "Build with moxygen_adapter and full subscriber tests (requires getdeps build)"
    ON)
message(STATUS "WITH_MOXYGEN_ADAPTER: ${WITH_MOXYGEN_ADAPTER}")
# -----------------------------------------------------------------------------

# ----- Sanitizer support -----------------------------------------------------
set(SANITIZE "" CACHE STRING "Enable a sanitizer: address, thread, undefined (default: none)")
if(SANITIZE)
    message(STATUS "Sanitizer: ${SANITIZE}")
    add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer -g -O1)
    add_link_options(-fsanitize=${SANITIZE})
endif()
# -----------------------------------------------------------------------------

# ----- Locate getdeps install tree -------------------------------------------
set(MOXYGEN_DIR ${CMAKE_SOURCE_DIR}/../moqt_implementation/moxygen/moxygen)
set(GETDEPS_PY  ${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py)

if(WITH_MOXYGEN_ADAPTER)
    # Full install dir gives us the moxygen lib AND the dep tree root
    execute_process(
        COMMAND ${GETDEPS_PY} show-inst-dir moxygen
        OUTPUT_VARIABLE MOXYGEN_INSTALL_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT MOXYGEN_INSTALL_DIR)
        message(FATAL_ERROR "Could not find moxygen install dir via getdeps. "
            "Run moqt_implementation/moxygen/scripts/setup-deps.sh first.")
    endif()
    message(STATUS "Moxygen install dir: ${MOXYGEN_INSTALL_DIR}")
    get_filename_component(DEPS_BASE_DIR ${MOXYGEN_INSTALL_DIR} DIRECTORY)
else()
    # No moxygen, but folly is still needed for base/ and the core tests
    execute_process(
        COMMAND ${GETDEPS_PY} show-inst-dir folly
        OUTPUT_VARIABLE FOLLY_INSTALL_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT FOLLY_INSTALL_DIR)
        message(FATAL_ERROR "Could not find folly install dir via getdeps. "
            "Run moqt_implementation/moxygen/scripts/setup-deps.sh first.")
    endif()
    message(STATUS "Folly install dir: ${FOLLY_INSTALL_DIR} (core build, no moxygen)")
    get_filename_component(DEPS_BASE_DIR ${FOLLY_INSTALL_DIR} DIRECTORY)
endif()
# -----------------------------------------------------------------------------

# ----- Common deps (always needed: folly, glog, gflags, fmt, ...) -----------
file(GLOB GLOG_DIR              ${DEPS_BASE_DIR}/glog*)
file(GLOB GFLAGS_DIR            ${DEPS_BASE_DIR}/gflags*)
file(GLOB FMT_DIR               ${DEPS_BASE_DIR}/fmt*)
file(GLOB BOOST_DIR             ${DEPS_BASE_DIR}/boost*)
file(GLOB DOUBLE_CONVERSION_DIR ${DEPS_BASE_DIR}/double-conversion*)
file(GLOB OPENSSL_DIR           ${DEPS_BASE_DIR}/openssl*)
file(GLOB LIBEVENT_DIR          ${DEPS_BASE_DIR}/libevent*)

find_library(FOLLY_LIB          folly            PATHS ${DEPS_BASE_DIR}/folly/lib  NO_DEFAULT_PATH)
find_library(GLOG_LIB           glog             PATHS ${GLOG_DIR}/lib             NO_DEFAULT_PATH)
find_library(GFLAGS_LIB         gflags           PATHS ${GFLAGS_DIR}/lib           NO_DEFAULT_PATH)
find_library(FMT_LIB            fmt              PATHS ${FMT_DIR}/lib              NO_DEFAULT_PATH)
find_library(LIBEVENT_LIB       event            PATHS ${LIBEVENT_DIR}/lib         NO_DEFAULT_PATH)
find_library(DOUBLE_CONVERSION_LIB double-conversion PATHS ${DOUBLE_CONVERSION_DIR}/lib NO_DEFAULT_PATH)
find_library(BOOST_CONTEXT_LIB  boost_context    PATHS ${BOOST_DIR}/lib            NO_DEFAULT_PATH)
find_library(BOOST_SYSTEM_LIB   boost_system     PATHS ${BOOST_DIR}/lib            NO_DEFAULT_PATH)
find_library(BOOST_THREAD_LIB   boost_thread     PATHS ${BOOST_DIR}/lib            NO_DEFAULT_PATH)
# -----------------------------------------------------------------------------

# ----- Moxygen-stack deps (only needed with WITH_MOXYGEN_ADAPTER) ------------
if(WITH_MOXYGEN_ADAPTER)
    file(GLOB ZSTD_DIR  ${DEPS_BASE_DIR}/zstd*)
    file(GLOB LZ4_DIR   ${DEPS_BASE_DIR}/lz4*)

    file(GLOB MOXYGEN_ALL_LIBS "${MOXYGEN_INSTALL_DIR}/lib/libmoxygen*.a")
    file(GLOB MVFST_LIBS       "${DEPS_BASE_DIR}/mvfst/lib/libmvfst*.a")

    find_library(FIZZ_LIB             fizz             PATHS ${DEPS_BASE_DIR}/fizz/lib     NO_DEFAULT_PATH)
    find_library(PROXYGEN_LIB         proxygen         PATHS ${DEPS_BASE_DIR}/proxygen/lib NO_DEFAULT_PATH)
    find_library(QUIC_WEBTRANSPORT_LIB proxygen_http_webtransport_quicwebtransport
                                                       PATHS ${DEPS_BASE_DIR}/proxygen/lib NO_DEFAULT_PATH)
    find_library(WANGLE_LIB           wangle           PATHS ${DEPS_BASE_DIR}/wangle/lib   NO_DEFAULT_PATH)

    find_library(MOXYGEN_LIB moxygen_moq PATHS ${MOXYGEN_INSTALL_DIR}/lib NO_DEFAULT_PATH)
    if(NOT MOXYGEN_LIB)
        message(FATAL_ERROR "Moxygen library not found in ${MOXYGEN_INSTALL_DIR}/lib")
    endif()
    message(STATUS "Found moxygen library: ${MOXYGEN_LIB}")
endif()
# -----------------------------------------------------------------------------

# OpenSSL: try getdeps install, fall back to system
find_library(SSL_LIB    ssl    PATHS ${OPENSSL_DIR}/lib NO_DEFAULT_PATH)
find_library(CRYPTO_LIB crypto PATHS ${OPENSSL_DIR}/lib NO_DEFAULT_PATH)
if(NOT SSL_LIB)
    find_library(SSL_LIB    ssl)
endif()
if(NOT CRYPTO_LIB)
    find_library(CRYPTO_LIB crypto)
endif()

# libunwind (folly stack traces)
find_library(UNWIND_LIB unwind)

# liblzma — on Linux the getdeps-built libunwind.a references lzma_stream_buffer_decode.
# Ubuntu 22.04's linker (binutils 2.38+) requires all transitive DSO deps to be
# explicit on the command line; link lzma after unwind to satisfy this.
if(UNIX AND NOT APPLE)
    find_library(LZMA_LIB lzma)
    if(LZMA_LIB)
        message(STATUS "Found lzma library: ${LZMA_LIB}")
    endif()
endif()

# libiberty — on Linux, cplus_demangle_v3_callback and rust_demangle_callback
# are provided by stub_demangle.c (compiled directly into the binary) so that
# the Docker build does not need either libiberty-dev or the Rust toolchain.
# The stubs are no-ops that leave symbol names undemangled in stack traces;
# this has no effect on test correctness.

# lz4 (needed only by moxygen stack, but found here for include dir)
set(LZ4_LIB "")
if(WITH_MOXYGEN_ADAPTER)
    file(GLOB LZ4_DIRS "${DEPS_BASE_DIR}/lz4*")
    foreach(LZ4_CANDIDATE_DIR ${LZ4_DIRS})
        find_library(LZ4_LIB_CANDIDATE lz4 PATHS ${LZ4_CANDIDATE_DIR}/lib NO_DEFAULT_PATH)
        if(LZ4_LIB_CANDIDATE)
            set(LZ4_LIB  ${LZ4_LIB_CANDIDATE})
            set(LZ4_DIR  ${LZ4_CANDIDATE_DIR})  # used for include path below
            message(STATUS "Found lz4 library: ${LZ4_LIB}")
            break()
        endif()
        unset(LZ4_LIB_CANDIDATE CACHE)
    endforeach()
    if(NOT LZ4_LIB)
        find_library(LZ4_LIB lz4)
    endif()
endif()

set(ZSTD_LIB "")
if(WITH_MOXYGEN_ADAPTER)
    file(GLOB ZSTD_DIRS "${DEPS_BASE_DIR}/zstd*")
    foreach(ZSTD_CANDIDATE_DIR ${ZSTD_DIRS})
        find_library(ZSTD_LIB_CANDIDATE zstd PATHS ${ZSTD_CANDIDATE_DIR}/lib NO_DEFAULT_PATH)
        if(ZSTD_LIB_CANDIDATE)
            set(ZSTD_LIB ${ZSTD_LIB_CANDIDATE})
            message(STATUS "Found zstd library: ${ZSTD_LIB}")
            break()
        endif()
        unset(ZSTD_LIB_CANDIDATE CACHE)
    endforeach()
    if(NOT ZSTD_LIB)
        message(WARNING "zstd library not found in dependencies, trying system-wide")
        find_library(ZSTD_LIB zstd)
    endif()
endif()

# Crypto libs used by fizz/mvfst (only needed with moxygen stack)
set(SODIUM_LIB "")
set(OQS_LIB    "")
if(WITH_MOXYGEN_ADAPTER)
    file(GLOB LIBSODIUM_DIRS "${DEPS_BASE_DIR}/libsodium-*")
    file(GLOB LIBOQS_DIRS    "${DEPS_BASE_DIR}/liboqs-*")

    foreach(LIBSODIUM_CANDIDATE_DIR ${LIBSODIUM_DIRS})
        find_library(SODIUM_LIB_CANDIDATE sodium PATHS ${LIBSODIUM_CANDIDATE_DIR}/lib NO_DEFAULT_PATH)
        if(SODIUM_LIB_CANDIDATE)
            set(SODIUM_LIB ${SODIUM_LIB_CANDIDATE})
            message(STATUS "Found sodium library: ${SODIUM_LIB}")
            break()
        endif()
        unset(SODIUM_LIB_CANDIDATE CACHE)
    endforeach()
    if(NOT SODIUM_LIB)
        find_library(SODIUM_LIB sodium)
    endif()

    foreach(LIBOQS_CANDIDATE_DIR ${LIBOQS_DIRS})
        find_library(OQS_LIB_CANDIDATE oqs PATHS ${LIBOQS_CANDIDATE_DIR}/lib NO_DEFAULT_PATH)
        if(OQS_LIB_CANDIDATE)
            set(OQS_LIB ${OQS_LIB_CANDIDATE})
            message(STATUS "Found oqs library: ${OQS_LIB}")
            break()
        endif()
        unset(OQS_LIB_CANDIDATE CACHE)
    endforeach()
    if(NOT OQS_LIB)
        find_library(OQS_LIB oqs)
    endif()
endif()

# ----- Source lists ----------------------------------------------------------
# Core: compiles without moxygen (folly only)
set(CORE_SOURCES
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
    # These two subscriber tests use only the abstract interface (no moxygen types)
    subscriber_tests/fetch_test.cpp
    subscriber_tests/subscribe_namespace_test.cpp
)

# On Linux: add a no-op stub for rust_demangle_callback so that the linker is
# satisfied even when the Rust toolchain is absent from the Docker build image.
# folly is compiled with FOLLY_HAVE_RUST_DEMANGLE=1 by getdeps whenever CMake
# detects a rustc binary; without the stub the final link would fail with an
# undefined reference to `rust_demangle_callback'.
if(UNIX AND NOT APPLE)
    list(APPEND CORE_SOURCES stub_demangle.c)
endif()

# Moxygen-dependent: include <moxygen/MoQConsumers.h> directly
set(MOXYGEN_SUBSCRIBER_SOURCES
    subscriber_tests/subscribe_test.cpp
    subscriber_tests/subscribe_error_test.cpp
    subscriber_tests/subscribe_update_test.cpp
    subscriber_tests/subscribe_update_error_test.cpp
    subscriber_tests/track_status_test.cpp
)

# The moxygen adapter itself
set(MOXYGEN_ADAPTER_SOURCES
    moxygen_adapter/moxygen_interface.cpp
    moxygen_adapter/moxygen_mocks.cpp
    moxygen_adapter/moxygen_fixture.cpp
)

if(WITH_MOXYGEN_ADAPTER)
    list(APPEND CORE_SOURCES ${MOXYGEN_SUBSCRIBER_SOURCES} ${MOXYGEN_ADAPTER_SOURCES})
endif()
# -----------------------------------------------------------------------------

add_executable(interop_tests ${CORE_SOURCES})

# Propagate adapter flag to sources (used in test_suite.cpp)
target_compile_definitions(interop_tests PRIVATE
    $<$<BOOL:${WITH_MOXYGEN_ADAPTER}>:WITH_MOXYGEN_ADAPTER>)

# ----- Include directories ---------------------------------------------------
target_include_directories(interop_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DEPS_BASE_DIR}/folly/include
    ${BOOST_DIR}/include
    ${DOUBLE_CONVERSION_DIR}/include
    ${LIBEVENT_DIR}/include
    ${OPENSSL_DIR}/include
    ${GLOG_DIR}/include
    ${GFLAGS_DIR}/include
    ${FMT_DIR}/include
)

if(WITH_MOXYGEN_ADAPTER)
    target_include_directories(interop_tests PRIVATE
        ${MOXYGEN_DIR}
        ${MOXYGEN_INSTALL_DIR}/include
        ${DEPS_BASE_DIR}/fizz/include
        ${DEPS_BASE_DIR}/proxygen/include
        ${DEPS_BASE_DIR}/wangle/include
        ${DEPS_BASE_DIR}/mvfst/include
    )
    # zstd include (versioned dir)
    foreach(ZSTD_CANDIDATE_DIR ${ZSTD_DIRS})
        if(EXISTS ${ZSTD_CANDIDATE_DIR}/include)
            target_include_directories(interop_tests PRIVATE ${ZSTD_CANDIDATE_DIR}/include)
            break()
        endif()
    endforeach()
    # lz4 include
    foreach(LZ4_CANDIDATE_DIR ${LZ4_DIRS})
        if(EXISTS ${LZ4_CANDIDATE_DIR}/include)
            target_include_directories(interop_tests PRIVATE ${LZ4_CANDIDATE_DIR}/include)
            break()
        endif()
    endforeach()
endif()
# -----------------------------------------------------------------------------

# ----- Link libraries --------------------------------------------------------
# GNU ld (Linux, binutils 2.38+) processes static archives left-to-right in a
# single pass and applies --as-needed to shared libs.  The Meta stack has
# bidirectional deps (moxygen→folly, fizz→folly, libunwind→zlib/lzma, etc.).
# Wrapping all archives in --start-group/--end-group makes ld re-scan the
# group until every symbol is resolved.  macOS's ld handles this automatically.

# ── zlib (needed by getdeps-built libunwind for decompression) ────────────────
file(GLOB ZLIB_DIR   ${DEPS_BASE_DIR}/zlib*)
find_library(ZLIB_LIB z PATHS ${ZLIB_DIR}/lib NO_DEFAULT_PATH)
if(NOT ZLIB_LIB)
    find_library(ZLIB_LIB z)
endif()
if(ZLIB_LIB)
    message(STATUS "Found zlib library: ${ZLIB_LIB}")
endif()

# ── Accumulate all static archives into META_ARCHIVES ─────────────────────────
set(META_ARCHIVES
    ${FOLLY_LIB}
    ${GLOG_LIB}
    ${GFLAGS_LIB}
    ${FMT_LIB}
    ${LIBEVENT_LIB}
    ${DOUBLE_CONVERSION_LIB}
    ${BOOST_CONTEXT_LIB}
    ${BOOST_SYSTEM_LIB}
    ${BOOST_THREAD_LIB}
)

if(UNWIND_LIB)
    list(APPEND META_ARCHIVES ${UNWIND_LIB})
endif()
if(ZLIB_LIB)
    list(APPEND META_ARCHIVES ${ZLIB_LIB})
endif()
if(LZMA_LIB)
    list(APPEND META_ARCHIVES ${LZMA_LIB})
endif()
if(SSL_LIB)
    list(APPEND META_ARCHIVES ${SSL_LIB})
endif()
if(CRYPTO_LIB)
    list(APPEND META_ARCHIVES ${CRYPTO_LIB})
endif()

if(WITH_MOXYGEN_ADAPTER)
    list(APPEND META_ARCHIVES
        ${MOXYGEN_ALL_LIBS}
        ${PROXYGEN_LIB}
        ${MVFST_LIBS}
        ${WANGLE_LIB}
        ${FIZZ_LIB}
        ${ZSTD_LIB}
        ${LZ4_LIB}
    )
    if(QUIC_WEBTRANSPORT_LIB)
        list(APPEND META_ARCHIVES ${QUIC_WEBTRANSPORT_LIB})
    endif()
    if(SODIUM_LIB)
        list(APPEND META_ARCHIVES ${SODIUM_LIB})
    endif()
    if(OQS_LIB)
        list(APPEND META_ARCHIVES ${OQS_LIB})
    endif()
endif()

# ── Link with group wrapping on Linux, plain on macOS ─────────────────────────
if(UNIX AND NOT APPLE)
    target_link_libraries(interop_tests
        -Wl,--start-group ${META_ARCHIVES} -Wl,--end-group)
else()
    target_link_libraries(interop_tests ${META_ARCHIVES})
endif()

# ── System libraries (dynamic, outside the archive group) ─────────────────────
target_link_libraries(interop_tests pthread dl)
if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    target_link_libraries(interop_tests c++abi)
endif()
# -----------------------------------------------------------------------------

set_target_properties(interop_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
