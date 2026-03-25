cmake_minimum_required(VERSION 3.16)
project(interop_tests)

# =============================================================================
# Docker-specific CMakeLists
#
# Assumptions (always true inside the Docker builder stage):
#   • Linux / aarch64 or x86_64 — no macOS conditionals needed
#   • Full moxygen stack is present — WITH_MOXYGEN_ADAPTER is always ON
#   • getdeps installed everything into DEPS_BASE_DIR
#   • stub_demangle.c satisfies folly's rust/cplus demangler references
#   • GNU ld requires --start-group / --end-group for the archive group
#
# For local macOS / general builds use the parent CMakeLists.txt instead.
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sanitizer support (used by the moxygen-san / moxygen-tsan Docker presets)
set(SANITIZE "" CACHE STRING "Enable sanitizer: address, thread (default: none)")
if(SANITIZE)
    message(STATUS "Sanitizer: ${SANITIZE}")
    add_compile_options(-fsanitize=${SANITIZE} -fno-omit-frame-pointer -g -O1)
    add_link_options(-fsanitize=${SANITIZE})
endif()

# Source root — one level up from this docker/ subdirectory
set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# =============================================================================
# Locate getdeps install tree
# =============================================================================
# moqt_implementation/ sits alongside moq-interop_tests/, i.e. two levels up
# from this docker/ subdirectory: docker/ -> moq-interop_tests/ -> workspace/
set(MOXYGEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../moqt_implementation/moxygen/moxygen)
set(GETDEPS_PY  ${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py)

execute_process(
    COMMAND python3 ${GETDEPS_PY} show-inst-dir moxygen
    OUTPUT_VARIABLE MOXYGEN_INSTALL_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT MOXYGEN_INSTALL_DIR)
    message(FATAL_ERROR
        "Could not find moxygen install dir via getdeps.\n"
        "Run moqt_implementation/moxygen/scripts/setup-deps.sh first.")
endif()
message(STATUS "Moxygen install dir: ${MOXYGEN_INSTALL_DIR}")
get_filename_component(DEPS ${MOXYGEN_INSTALL_DIR} DIRECTORY)

# =============================================================================
# Library discovery (getdeps paths only — no system fallbacks)
# =============================================================================

file(GLOB GLOG_DIR              ${DEPS}/glog*)
file(GLOB GFLAGS_DIR            ${DEPS}/gflags*)
file(GLOB FMT_DIR               ${DEPS}/fmt*)
file(GLOB BOOST_DIR             ${DEPS}/boost*)
file(GLOB DOUBLE_CONVERSION_DIR ${DEPS}/double-conversion*)
file(GLOB OPENSSL_DIR           ${DEPS}/openssl*)
file(GLOB LIBEVENT_DIR          ${DEPS}/libevent*)
file(GLOB ZLIB_DIR              ${DEPS}/zlib*)
file(GLOB LZ4_DIRS              ${DEPS}/lz4*)
file(GLOB ZSTD_DIRS             ${DEPS}/zstd*)
file(GLOB LIBSODIUM_DIRS        ${DEPS}/libsodium-*)
file(GLOB LIBOQS_DIRS           ${DEPS}/liboqs-*)

find_library(FOLLY_LIB             folly            PATHS ${DEPS}/folly/lib                NO_DEFAULT_PATH REQUIRED)
find_library(GLOG_LIB              glog             PATHS ${GLOG_DIR}/lib                  NO_DEFAULT_PATH REQUIRED)
find_library(GFLAGS_LIB            gflags           PATHS ${GFLAGS_DIR}/lib                NO_DEFAULT_PATH REQUIRED)
find_library(FMT_LIB               fmt              PATHS ${FMT_DIR}/lib                   NO_DEFAULT_PATH REQUIRED)
find_library(LIBEVENT_LIB          event            PATHS ${LIBEVENT_DIR}/lib              NO_DEFAULT_PATH REQUIRED)
find_library(DOUBLE_CONVERSION_LIB double-conversion PATHS ${DOUBLE_CONVERSION_DIR}/lib    NO_DEFAULT_PATH REQUIRED)
find_library(BOOST_CONTEXT_LIB     boost_context    PATHS ${BOOST_DIR}/lib                NO_DEFAULT_PATH REQUIRED)
find_library(BOOST_SYSTEM_LIB      boost_system     PATHS ${BOOST_DIR}/lib                NO_DEFAULT_PATH REQUIRED)
find_library(BOOST_THREAD_LIB      boost_thread     PATHS ${BOOST_DIR}/lib                NO_DEFAULT_PATH REQUIRED)
find_library(FIZZ_LIB              fizz             PATHS ${DEPS}/fizz/lib                 NO_DEFAULT_PATH REQUIRED)
find_library(PROXYGEN_LIB          proxygen         PATHS ${DEPS}/proxygen/lib             NO_DEFAULT_PATH REQUIRED)
find_library(WANGLE_LIB            wangle           PATHS ${DEPS}/wangle/lib               NO_DEFAULT_PATH REQUIRED)
find_library(SSL_LIB               ssl              PATHS ${OPENSSL_DIR}/lib               NO_DEFAULT_PATH)
find_library(CRYPTO_LIB            crypto           PATHS ${OPENSSL_DIR}/lib               NO_DEFAULT_PATH)
# liboqs references OpenSSL 3.x symbols (EVP_CIPHER_fetch etc.).  If getdeps
# does not install a private OpenSSL, fall back to the system package.
if(NOT SSL_LIB)
    find_library(SSL_LIB ssl REQUIRED)
endif()
if(NOT CRYPTO_LIB)
    find_library(CRYPTO_LIB crypto REQUIRED)
endif()
find_library(ZLIB_LIB              z                PATHS ${ZLIB_DIR}/lib                  NO_DEFAULT_PATH)
find_library(UNWIND_LIB            unwind)
find_library(LZMA_LIB              lzma)

file(GLOB MOXYGEN_ALL_LIBS "${MOXYGEN_INSTALL_DIR}/lib/libmoxygen*.a")
file(GLOB MVFST_LIBS       "${DEPS}/mvfst/lib/libmvfst*.a")

# Optional: lz4
foreach(D ${LZ4_DIRS})
    find_library(LZ4_LIB lz4 PATHS ${D}/lib NO_DEFAULT_PATH)
    if(LZ4_LIB)
        set(LZ4_INC_DIR ${D})
        break()
    endif()
    unset(LZ4_LIB CACHE)
endforeach()

# Optional: zstd
foreach(D ${ZSTD_DIRS})
    find_library(ZSTD_LIB zstd PATHS ${D}/lib NO_DEFAULT_PATH)
    if(ZSTD_LIB)
        set(ZSTD_INC_DIR ${D})
        break()
    endif()
    unset(ZSTD_LIB CACHE)
endforeach()

# Optional: sodium
foreach(D ${LIBSODIUM_DIRS})
    find_library(SODIUM_LIB sodium PATHS ${D}/lib NO_DEFAULT_PATH)
    if(SODIUM_LIB)
        break()
    endif()
    unset(SODIUM_LIB CACHE)
endforeach()

# Optional: oqs
foreach(D ${LIBOQS_DIRS})
    find_library(OQS_LIB oqs PATHS ${D}/lib NO_DEFAULT_PATH)
    if(OQS_LIB)
        break()
    endif()
    unset(OQS_LIB CACHE)
endforeach()

# Optional: proxygen webtransport helper
find_library(QUIC_WEBTRANSPORT_LIB proxygen_http_webtransport_quicwebtransport
    PATHS ${DEPS}/proxygen/lib NO_DEFAULT_PATH)

# =============================================================================
# Sources
# =============================================================================

add_executable(interop_tests
    ${SRC}/main.cpp
    ${SRC}/test_suite.cpp
    ${SRC}/test_registry.cpp
    # Base / core
    ${SRC}/base/base_test.cpp
    ${SRC}/base/moqt_interface.cpp
    # Publisher tests
    ${SRC}/publisher_tests/publish_test.cpp
    ${SRC}/publisher_tests/publish_namespace_test.cpp
    ${SRC}/publisher_tests/publish_namespace_done_test.cpp
    ${SRC}/publisher_tests/goaway_test.cpp
    ${SRC}/publisher_tests/max_request_id_test.cpp
    # Subscriber tests
    ${SRC}/subscriber_tests/fetch_test.cpp
    ${SRC}/subscriber_tests/subscribe_namespace_test.cpp
    ${SRC}/subscriber_tests/subscribe_test.cpp
    ${SRC}/subscriber_tests/subscribe_error_test.cpp
    ${SRC}/subscriber_tests/subscribe_update_test.cpp
    ${SRC}/subscriber_tests/subscribe_update_error_test.cpp
    ${SRC}/subscriber_tests/track_status_test.cpp
    # Moxygen adapter
    ${SRC}/moxygen_adapter/moxygen_interface.cpp
    ${SRC}/moxygen_adapter/moxygen_mocks.cpp
    ${SRC}/moxygen_adapter/moxygen_fixture.cpp
    # No-op demangling stubs — satisfy folly's linker refs to
    # rust_demangle_callback and cplus_demangle_v3_callback without
    # needing a Rust toolchain or libiberty in the build image.
    ${SRC}/stub_demangle.c
)

target_compile_definitions(interop_tests PRIVATE WITH_MOXYGEN_ADAPTER)

# =============================================================================
# Include directories
# =============================================================================

target_include_directories(interop_tests PRIVATE
    ${SRC}
    ${DEPS}/folly/include
    ${BOOST_DIR}/include
    ${DOUBLE_CONVERSION_DIR}/include
    ${LIBEVENT_DIR}/include
    ${OPENSSL_DIR}/include
    ${GLOG_DIR}/include
    ${GFLAGS_DIR}/include
    ${FMT_DIR}/include
    ${MOXYGEN_DIR}
    ${MOXYGEN_INSTALL_DIR}/include
    ${DEPS}/fizz/include
    ${DEPS}/proxygen/include
    ${DEPS}/wangle/include
    ${DEPS}/mvfst/include
)

if(DEFINED LZ4_INC_DIR AND EXISTS ${LZ4_INC_DIR}/include)
    target_include_directories(interop_tests PRIVATE ${LZ4_INC_DIR}/include)
endif()
if(DEFINED ZSTD_INC_DIR AND EXISTS ${ZSTD_INC_DIR}/include)
    target_include_directories(interop_tests PRIVATE ${ZSTD_INC_DIR}/include)
endif()

# =============================================================================
# Link libraries
# GNU ld requires --start-group / --end-group to resolve bidirectional
# deps across the static archive group (folly↔fizz, unwind→lzma, etc.).
# =============================================================================

set(ARCHIVES
    ${MOXYGEN_ALL_LIBS}
    ${PROXYGEN_LIB}
    ${MVFST_LIBS}
    ${WANGLE_LIB}
    ${FIZZ_LIB}
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

foreach(OPT_LIB ZSTD_LIB LZ4_LIB UNWIND_LIB ZLIB_LIB LZMA_LIB
                SSL_LIB CRYPTO_LIB QUIC_WEBTRANSPORT_LIB SODIUM_LIB OQS_LIB)
    if(${OPT_LIB})
        list(APPEND ARCHIVES ${${OPT_LIB}})
    endif()
endforeach()

target_link_libraries(interop_tests
    -Wl,--start-group ${ARCHIVES} -Wl,--end-group
    pthread dl)

set_target_properties(interop_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
