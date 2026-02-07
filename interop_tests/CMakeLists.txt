cmake_minimum_required(VERSION 3.16)
project(interop_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use moxygen library directly
message(STATUS "Building with native moxygen library")

# Set moxygen paths
set(MOXYGEN_DIR ${CMAKE_SOURCE_DIR}/../moqt_implementation/moxygen)

# Get moxygen installation directory dynamically
execute_process(
    COMMAND ${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py show-inst-dir moxygen
    OUTPUT_VARIABLE MOXYGEN_INSTALL_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Moxygen install dir: ${MOXYGEN_INSTALL_DIR}")

# Get the dependencies installation base directory
get_filename_component(DEPS_BASE_DIR ${MOXYGEN_INSTALL_DIR} DIRECTORY)

# Find the built moxygen library directly
find_library(MOXYGEN_LIB moxygen PATHS ${MOXYGEN_INSTALL_DIR}/lib NO_DEFAULT_PATH)
if(NOT MOXYGEN_LIB)
    message(FATAL_ERROR "Moxygen library not found in ${MOXYGEN_INSTALL_DIR}/lib")
endif()
message(STATUS "Found moxygen library: ${MOXYGEN_LIB}")

# Find dependency libraries with exact directory matching
file(GLOB GLOG_DIR ${DEPS_BASE_DIR}/glog*)
file(GLOB GFLAGS_DIR ${DEPS_BASE_DIR}/gflags*)
file(GLOB FMT_DIR ${DEPS_BASE_DIR}/fmt*)
file(GLOB BOOST_DIR ${DEPS_BASE_DIR}/boost*)
file(GLOB DOUBLE_CONVERSION_DIR ${DEPS_BASE_DIR}/double-conversion*)
file(GLOB OPENSSL_DIR ${DEPS_BASE_DIR}/openssl*)
file(GLOB ZSTD_DIR ${DEPS_BASE_DIR}/zstd*)
file(GLOB LZ4_DIR ${DEPS_BASE_DIR}/lz4*)
file(GLOB LIBEVENT_DIR ${DEPS_BASE_DIR}/libevent*)

# Find main libraries
# Find all moxygen libraries
find_library(MOXYGEN_LIB moxygen PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(MOQRELAY_LIB moqrelay PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(MOQCACHE_LIB moqcache PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(MLOGGER_LIB mlogger PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(FLVPARSER_LIB flvparser PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(MOQMI_LIB moqmi PATHS ${DEPS_BASE_DIR}/moxygen/lib NO_DEFAULT_PATH)
find_library(FOLLY_LIB folly PATHS ${DEPS_BASE_DIR}/folly/lib NO_DEFAULT_PATH)
find_library(FIZZ_LIB fizz PATHS ${DEPS_BASE_DIR}/fizz/lib NO_DEFAULT_PATH)
find_library(PROXYGEN_LIB proxygen PATHS ${DEPS_BASE_DIR}/proxygen/lib NO_DEFAULT_PATH)
find_library(QUIC_WEBTRANSPORT_LIB quicwebtransport PATHS ${DEPS_BASE_DIR}/proxygen/lib NO_DEFAULT_PATH)
find_library(WANGLE_LIB wangle PATHS ${DEPS_BASE_DIR}/wangle/lib NO_DEFAULT_PATH)
# Find ALL mvfst libraries using GLOB
file(GLOB MVFST_LIBS "${DEPS_BASE_DIR}/mvfst/lib/libmvfst*.a")
find_library(GLOG_LIB glog PATHS ${GLOG_DIR}/lib NO_DEFAULT_PATH)
find_library(GFLAGS_LIB gflags PATHS ${GFLAGS_DIR}/lib NO_DEFAULT_PATH)
find_library(FMT_LIB fmt PATHS ${FMT_DIR}/lib NO_DEFAULT_PATH)
find_library(LIBEVENT_LIB event PATHS ${LIBEVENT_DIR}/lib NO_DEFAULT_PATH)
find_library(DOUBLE_CONVERSION_LIB double-conversion PATHS ${DOUBLE_CONVERSION_DIR}/lib NO_DEFAULT_PATH)

# Find additional libraries that might be needed
find_library(SSL_LIB ssl PATHS ${OPENSSL_DIR}/lib NO_DEFAULT_PATH)
find_library(CRYPTO_LIB crypto PATHS ${OPENSSL_DIR}/lib NO_DEFAULT_PATH)

# Find lz4 library with more flexible pattern matching
file(GLOB LZ4_DIRS "${DEPS_BASE_DIR}/lz4*")
set(LZ4_LIB "")
foreach(LZ4_CANDIDATE_DIR ${LZ4_DIRS})
    find_library(LZ4_LIB_CANDIDATE lz4 PATHS ${LZ4_CANDIDATE_DIR}/lib NO_DEFAULT_PATH)
    if(LZ4_LIB_CANDIDATE)
        set(LZ4_LIB ${LZ4_LIB_CANDIDATE})
        message(STATUS "Found lz4 library: ${LZ4_LIB}")
        break()
    endif()
    unset(LZ4_LIB_CANDIDATE CACHE)
endforeach()

if(NOT LZ4_LIB)
    message(WARNING "lz4 library not found in dependencies, trying system-wide")
    find_library(LZ4_LIB lz4)
endif()

# Find zstd library with more flexible pattern matching
file(GLOB ZSTD_DIRS "${DEPS_BASE_DIR}/zstd*")
set(ZSTD_LIB "")
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

# Find crypto libraries dynamically
file(GLOB LIBSODIUM_DIRS "${DEPS_BASE_DIR}/libsodium-*")
file(GLOB LIBOQS_DIRS "${DEPS_BASE_DIR}/liboqs-*")

# Find libsodium library with better error handling
set(SODIUM_LIB "")
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
    message(WARNING "libsodium library not found in dependencies, trying system-wide")
    find_library(SODIUM_LIB sodium)
endif()

# Find liboqs library with better error handling
set(OQS_LIB "")
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
    message(WARNING "liboqs library not found in dependencies, trying system-wide")
    find_library(OQS_LIB oqs)
endif()

# Find boost libraries
find_library(BOOST_CONTEXT_LIB boost_context PATHS ${BOOST_DIR}/lib NO_DEFAULT_PATH)
find_library(BOOST_SYSTEM_LIB boost_system PATHS ${BOOST_DIR}/lib NO_DEFAULT_PATH)
find_library(BOOST_THREAD_LIB boost_thread PATHS ${BOOST_DIR}/lib NO_DEFAULT_PATH)

# Add moxygen source files we need
set(MOXYGEN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../moqt_implementation/moxygen/moxygen)
set(MOXYGEN_SOURCES
    ${MOXYGEN_SRC_DIR}/MoQClient.cpp
    ${MOXYGEN_SRC_DIR}/MoQWebTransportClient.cpp
    ${MOXYGEN_SRC_DIR}/MoQClientBase.cpp
    ${MOXYGEN_SRC_DIR}/MoQRelaySession.cpp
    ${MOXYGEN_SRC_DIR}/events/MoQFollyExecutorImpl.cpp
    ${MOXYGEN_SRC_DIR}/util/QuicConnector.cpp
)

add_executable(interop_tests
    main.cpp
    test_suite.cpp
    test_registry.cpp
    publisher_tests/publish_test.cpp
    publisher_tests/publish_namespace_test.cpp
    publisher_tests/publish_namespace_done_test.cpp
    publisher_tests/goaway_test.cpp
    publisher_tests/max_request_id_test.cpp
    subscriber_tests/subscribe_test.cpp
    subscriber_tests/subscribe_error_test.cpp
    subscriber_tests/subscribe_namespace_test.cpp
    subscriber_tests/subscribe_namespace_error_test.cpp
    subscriber_tests/subscribe_update_test.cpp
    subscriber_tests/subscribe_update_error_test.cpp
    subscriber_tests/track_status_test.cpp
    subscriber_tests/fetch_test.cpp
    moxygen_adapter/moxygen_interface.cpp
    moxygen_adapter/moxygen_mocks.cpp
    moxygen_adapter/moxygen_fixture.cpp
    base/base_test.cpp
    base/moqt_interface.cpp
    ${MOXYGEN_SOURCES}
)

# Include moxygen and dependency headers
target_include_directories(interop_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}    
    ${MOXYGEN_DIR}
    ${MOXYGEN_INSTALL_DIR}/include
    ${DEPS_BASE_DIR}/folly/include
    ${DEPS_BASE_DIR}/fizz/include
    ${DEPS_BASE_DIR}/proxygen/include
    ${DEPS_BASE_DIR}/wangle/include
    ${DEPS_BASE_DIR}/mvfst/include
    ${BOOST_DIR}/include
    ${DOUBLE_CONVERSION_DIR}/include
    ${OPENSSL_DIR}/include
    ${LZ4_DIR}/include
    ${LIBEVENT_DIR}/include
)

# Add zstd include directory dynamically
foreach(ZSTD_CANDIDATE_DIR ${ZSTD_DIRS})
    if(EXISTS ${ZSTD_CANDIDATE_DIR}/include)
        target_include_directories(interop_tests PRIVATE ${ZSTD_CANDIDATE_DIR}/include)
        break()
    endif()
endforeach()

# Find additional include directories
target_include_directories(interop_tests PRIVATE
    ${GLOG_DIR}/include
    ${GFLAGS_DIR}/include
    ${FMT_DIR}/include
)

# Link against all required libraries - order matters for static libraries
target_link_libraries(interop_tests
    # Start with moxygen libraries in dependency order
    ${MOXYGEN_LIB}
    ${MOQRELAY_LIB}
    ${MOQCACHE_LIB}
    ${MLOGGER_LIB}
    ${MOQMI_LIB}
    ${FLVPARSER_LIB}
    # Then mvfst libraries
    ${MVFST_LIBS}
    # Then the rest in dependency order
    ${PROXYGEN_LIB}
    ${QUIC_WEBTRANSPORT_LIB}
    ${WANGLE_LIB}
    ${FIZZ_LIB}
    ${FOLLY_LIB}
    ${GLOG_LIB}
    ${GFLAGS_LIB}
    ${FMT_LIB}
    ${LIBEVENT_LIB}
    ${DOUBLE_CONVERSION_LIB}
    ${SSL_LIB}
    ${CRYPTO_LIB}
    ${ZSTD_LIB}
    ${LZ4_LIB}
    ${BOOST_CONTEXT_LIB}
    ${BOOST_SYSTEM_LIB}
    ${BOOST_THREAD_LIB}
    ${SODIUM_LIB}
    ${OQS_LIB}
    pthread
    dl
    c++abi
)

set_target_properties(interop_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
