# Install script for directory: /Users/akashn/Projects/openmoq/moq-relay-test/test_framework

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/akashn/Projects/openmoq/moq-relay-test/test_framework/build/moq_test_runner")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZopenmoqZmoq-relay-testZmoqt_implementationZmoxygenZbuildZfbcode_builder/installed/gflags-R_ip761XFbPRaHAxhqmkshaiVFY8VrKHt-RKoSLk560/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZopenmoqZmoq-relay-testZmoqt_implementationZmoxygenZbuildZfbcode_builder/installed/gflags-R_ip761XFbPRaHAxhqmkshaiVFY8VrKHt-RKoSLk560/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/moq_test_runner")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/akashn/Projects/openmoq/moq-relay-test/test_framework/build/simple_example")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZopenmoqZmoq-relay-testZmoqt_implementationZmoxygenZbuildZfbcode_builder/installed/gflags-R_ip761XFbPRaHAxhqmkshaiVFY8VrKHt-RKoSLk560/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib:/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libevent-0od4V_3_RzIyUVnz-M9vMGpah7G-M-ny0bTjjUs38lA/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/liboqs-vQ_Kd_vpm2mo0001V1FBMlFU9-XPlYGUILGbV42RnZ4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-TFJnHsHi4Vy-6MFBM4_UxDRXhfjRwlNYuhIZv2W870U/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZopenmoqZmoq-relay-testZmoqt_implementationZmoxygenZbuildZfbcode_builder/installed/gflags-R_ip761XFbPRaHAxhqmkshaiVFY8VrKHt-RKoSLk560/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/gflags-gM09wdTE0OyGQJ5i8hqCZb8XtB9-IkIgfIefyXOeZBk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/wangle/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-aTcGddsCSHz5v0a46QECvZAh1NiQChig06FiUfeARdk/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fmt-oJ8YQ3wfIYKqRbKo9PBLZkoGcnn8xNmgie828R-jNpE/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/include/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/proxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/googletest-SpTfKj6Hnf5vxvBDB4R52yH0i3lNdgJmLmpTQI70pLg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-QGMfE7nhdXlum6PwXHd1NJd_6H6A-ya5Yh16tFUjb2A/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/moxygen/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/lz4-XAbAbI0NPZ3mNsRk0JKM4hE-MGsuaHbYe04nr9ov6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libsodium-7JMNfJsBnN93PhB8mumJ32EPZomjKML_p66ps0FhvRw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/glog-GFx2L8Mu_YDVA41We1XEB81TcnExZzhiW9NprGWq6tw/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/openssl-POQW-3g_K-grkgwxUNCXdu56Lz_r4n1drg33C33TNCM/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/boost-aRNp_zTGpjXT_956d84onGD4gJCuAQ8SBrCVvUP54-s/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zlib-PSY38PpiiPxOjlzd8HTD1-VEOTFXsQy6GSv1QE4K540/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/c-ares-3bhe-3vJRBpBrCDK3QPXojMZg1UkWWXTYOX1fvtbQZY/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-z6r_Bm0c_r6w9PHOFMDhULDSegWXVlxQt6Z22DtCjew/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libdwarf-q4JfuANVaTrEVxNULOLhnTgPXgqbSpNdj3VICcewgW8/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/zstd-kelfwaASUpe9YiEmL_nrCdbyhHrj1yskTfvYZ0Uydsg/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/mvfst/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/xz-Dbmw5kTS-Zb7IfuFEJ7tgijkcPSG7Bz26Af5NtpS6xs/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/double-conversion-IsrhMLWs3MCm7--QY9wSZmbuQjh94Z6HF3-3eQAoef4/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/folly/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/snappy-gceUB9XcJmOqRPNsLfLqk82RZls8g-kNQfM1rkXhFys/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/fizz/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    execute_process(COMMAND /usr/bin/install_name_tool
      -add_rpath "/private/var/folders/bz/sgnqm9ns47b6mkdtr_h1l8p80000gp/T/fbcode_builder_getdeps-ZUsersZakashnZProjectsZMoQZmoxygen_newZmoxygenZbuildZfbcode_builder/installed/libtool-GkDGK62px_7LVZjeFD8CLgAWlyEtMSapdf3eYbaWQ98/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/simple_example")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/akashn/Projects/openmoq/moq-relay-test/test_framework/build/libmoq_test_framework.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmoq_test_framework.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmoq_test_framework.a")
    execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libmoq_test_framework.a")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/Users/akashn/Projects/openmoq/moq-relay-test/test_framework/MoQServerTestFramework.h")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/akashn/Projects/openmoq/moq-relay-test/test_framework/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
