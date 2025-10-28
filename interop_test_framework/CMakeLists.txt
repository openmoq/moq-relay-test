cmake_minimum_required(VERSION 3.16)
project(interop_test_framework)

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
find_library(ZSTD_LIB zstd PATHS ${ZSTD_DIR}/lib NO_DEFAULT_PATH)
find_library(LZ4_LIB lz4 PATHS ${DEPS_BASE_DIR}/lz4-*/lib NO_DEFAULT_PATH)

# Find crypto libraries dynamically
file(GLOB LIBSODIUM_DIRS "${DEPS_BASE_DIR}/libsodium-*")
file(GLOB LIBOQS_DIRS "${DEPS_BASE_DIR}/liboqs-*")

if(LIBSODIUM_DIRS)
    list(GET LIBSODIUM_DIRS 0 LIBSODIUM_DIR)
    find_library(SODIUM_LIB sodium PATHS ${LIBSODIUM_DIR}/lib NO_DEFAULT_PATH)
endif()

if(LIBOQS_DIRS)
    list(GET LIBOQS_DIRS 0 LIBOQS_DIR)
    find_library(OQS_LIB oqs PATHS ${LIBOQS_DIR}/lib NO_DEFAULT_PATH)
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
    ${MOXYGEN_SRC_DIR}/events/MoQFollyExecutorImpl.cpp
    ${MOXYGEN_SRC_DIR}/util/QuicConnector.cpp
)

add_executable(interop_test_runner
    main.cpp
    PublishTest.cpp
    ${MOXYGEN_SOURCES}
)

# Include moxygen and dependency headers
target_include_directories(interop_test_runner PRIVATE
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
    ${ZSTD_DIR}/include
    ${LZ4_DIR}/include
    ${LIBEVENT_DIR}/include
)

# Find additional include directories
target_include_directories(interop_test_runner PRIVATE
    ${GLOG_DIR}/include
    ${GFLAGS_DIR}/include
    ${FMT_DIR}/include
)

# Link against all required libraries - order matters for static libraries
target_link_libraries(interop_test_runner
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

set_target_properties(interop_test_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)