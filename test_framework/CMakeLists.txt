# CMakeLists.txt for MOQ Server Test Framework

cmake_minimum_required(VERSION 3.16)
project(moq_test_framework)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Custom library search paths ===
# Ensures CMake and the runtime loader can find all dependencies
set(CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}" ${CMAKE_PREFIX_PATH})
set(CMAKE_LIBRARY_PATH "$ENV{LD_LIBRARY_PATH}" ${CMAKE_LIBRARY_PATH})

# List all library directories
set(MOXYGEN_LIB_PATHS
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib
    /private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib
)

# Build-time library lookup
link_directories(${MOXYGEN_LIB_PATHS})

# Runtime rpath embedding (so LD_LIBRARY_PATH isn't needed)
list(JOIN MOXYGEN_LIB_PATHS ":" MOXYGEN_RPATHS)
set(CMAKE_BUILD_RPATH "${MOXYGEN_RPATHS}")
set(CMAKE_INSTALL_RPATH "${MOXYGEN_RPATHS}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Find required packages for explicitly used dependencies
find_package(Threads REQUIRED)
find_package(gflags REQUIRED)

# set (CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib")
# Find moxygen - this should point to the moxygen installation
# You may need to adjust this path based on your moxygen build
set(MOXYGEN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../moqt_implementation/moxygen")


message(STATUS "Found moxygen library: ${MOXYGEN_LIB}")

# Create the test framework library
add_library(moq_test_framework STATIC
    MoQServerTestFramework.cpp
)

target_include_directories(moq_test_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MOXYGEN_ROOT}
    ${MOXYGEN_ROOT}/build
)

target_link_libraries(moq_test_framework PUBLIC
    ${MOXYGEN_LIB}
    folly::folly
    proxygen::proxygen
    gflags
    Threads::Threads
)

# Create the test runner executable
add_executable(moq_test_runner
    moq_test_runner.cpp
)

target_link_libraries(moq_test_runner PRIVATE
    moq_test_framework
)

# Create the simple example executable
add_executable(simple_example
    simple_example.cpp
)

target_link_libraries(simple_example PRIVATE
    moq_test_framework
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_compile_options(moq_test_framework PRIVATE -Wall -Wextra)
    target_compile_options(moq_test_runner PRIVATE -Wall -Wextra)
    target_compile_options(simple_example PRIVATE -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(moq_test_framework PRIVATE -Wall -Wextra)
    target_compile_options(moq_test_runner PRIVATE -Wall -Wextra)
    target_compile_options(simple_example PRIVATE -Wall -Wextra)
endif()

# Install targets
install(TARGETS moq_test_runner simple_example
    RUNTIME DESTINATION bin
)

install(TARGETS moq_test_framework
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES MoQServerTestFramework.h
    DESTINATION include
)

# Create a simple test to verify the framework builds
enable_testing()
add_test(NAME framework_builds
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target moq_test_framework)

# Print configuration summary
message(STATUS "")
message(STATUS "MOQ Test Framework Configuration Summary:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Moxygen Root: ${MOXYGEN_ROOT}")
message(STATUS "  Moxygen Library: ${MOXYGEN_LIB}")
message(STATUS "")