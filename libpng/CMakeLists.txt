# CMakeLists.txt

# Copyright (C) 2007,2009-2018 Glenn Randers-Pehrson
# Written by Christian Ehrlicher, 2007
# Revised by Roger Lowman, 2009-2010
# Revised by Clifford Yapp, 2011-2012
# Revised by Roger Leigh, 2016
# Revised by Andreas Franek, 2016

# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h

cmake_minimum_required(VERSION 3.0.2)
cmake_policy(VERSION 3.0.2)

# Set MacOSX @rpath usage globally.
if (POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif(POLICY CMP0020)
if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif(POLICY CMP0042)
# Use new variable expansion policy.
if (POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW)
endif(POLICY CMP0053)
if (POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif(POLICY CMP0054)

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;MinSizeRel;RelWithDebInfo")

project(libpng C ASM)
enable_testing()

set(PNGLIB_MAJOR 1)
set(PNGLIB_MINOR 6)
set(PNGLIB_RELEASE 35)
set(PNGLIB_NAME libpng${PNGLIB_MAJOR}${PNGLIB_MINOR})
set(PNGLIB_VERSION ${PNGLIB_MAJOR}.${PNGLIB_MINOR}.${PNGLIB_RELEASE})

include(GNUInstallDirs)

# needed packages
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR ../zlib)

if(NOT WIN32)
  find_library(M_LIBRARY
    NAMES m
    PATHS /usr/lib /usr/local/lib
  )
  if(NOT M_LIBRARY)
    message(STATUS "math lib 'libm' not found; floating point support disabled")
  endif()
else()
  # not needed on windows
  set(M_LIBRARY "")
endif()

# COMMAND LINE OPTIONS
option(PNG_SHARED "Build shared lib" OFF)
option(PNG_STATIC "Build static lib" ON)

# Many more configuration options could be added here
option(PNG_FRAMEWORK "Build OS X framework" OFF)
option(PNG_DEBUG     "Build with debug output" OFF)
option(PNGARG        "Disable ANSI-C prototypes" OFF)

option(PNG_HARDWARE_OPTIMIZATIONS "Enable Hardware Optimizations" ON)


set(PNG_PREFIX "" CACHE STRING "Prefix to add to the API function names")
set(DFA_XTRA "" CACHE FILEPATH "File containing extra configuration settings")

if(PNG_HARDWARE_OPTIMIZATIONS)
# set definitions and sources for arm
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR
  CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
  set(PNG_ARM_NEON_POSSIBLE_VALUES check on off)
  set(PNG_ARM_NEON "check" CACHE STRING "Enable ARM NEON optimizations:
     check: (default) use internal checking code;
     off: disable the optimizations;
     on: turn on unconditionally.")
  set_property(CACHE PNG_ARM_NEON PROPERTY STRINGS
     ${PNG_ARM_NEON_POSSIBLE_VALUES})
  list(FIND PNG_ARM_NEON_POSSIBLE_VALUES ${PNG_ARM_NEON} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_ARM_NEON must be one of [${PNG_ARM_NEON_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_ARM_NEON} STREQUAL "no")
    set(libpng_arm_sources
      arm/arm_init.c
      arm/filter_neon.S
      arm/filter_neon_intrinsics.c)

    if(${PNG_ARM_NEON} STREQUAL "on")
      add_definitions(-DPNG_ARM_NEON_OPT=2)
    elseif(${PNG_ARM_NEON} STREQUAL "check")
      add_definitions(-DPNG_ARM_NEON_CHECK_SUPPORTED)
    endif()
  else()
    add_definitions(-DPNG_ARM_NEON_OPT=0)
  endif()
endif()

# set definitions and sources for powerpc
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^powerpc*" OR
	CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc64*" )
  set(PNG_POWERPC_VSX_POSSIBLE_VALUES on off)
  set(PNG_POWERPC_VSX "on" CACHE STRING "Enable POWERPC VSX optimizations:
     off: disable the optimizations.")
  set_property(CACHE PNG_POWERPC_VSX PROPERTY STRINGS
     ${PNG_POWERPC_VSX_POSSIBLE_VALUES})
  list(FIND PNG_POWERPC_VSX_POSSIBLE_VALUES ${PNG_POWERPC_VSX} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_POWERPC_VSX must be one of [${PNG_POWERPC_VSX_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_POWERPC_VSX} STREQUAL "no")
    set(libpng_powerpc_sources
      powerpc/powerpc_init.c
      powerpc/filter_vsx_intrinsics.c)
    if(${PNG_POWERPC_VSX} STREQUAL "on")
      add_definitions(-DPNG_POWERPC_VSX_OPT=2)
    endif()
  else()
    add_definitions(-DPNG_POWERPC_VSX_OPT=0)
  endif()
endif()

# set definitions and sources for intel
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i?86" OR
	CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64*" )
  set(PNG_INTEL_SSE_POSSIBLE_VALUES on off)
  set(PNG_INTEL_SSE "on" CACHE STRING "Enable INTEL_SSE optimizations:
     off: disable the optimizations")
  set_property(CACHE PNG_INTEL_SSE PROPERTY STRINGS
     ${PNG_INTEL_SSE_POSSIBLE_VALUES})
  list(FIND PNG_INTEL_SSE_POSSIBLE_VALUES ${PNG_INTEL_SSE} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_INTEL_SSE must be one of [${PNG_INTEL_SSE_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_INTEL_SSE} STREQUAL "no")
    set(libpng_intel_sources
      intel/intel_init.c
      intel/filter_sse2_intrinsics.c)
    if(${PNG_INTEL_SSE} STREQUAL "on")
      add_definitions(-DPNG_INTEL_SSE_OPT=1)
    endif()
  else()
    add_definitions(-DPNG_INTEL_SSE_OPT=0)
  endif()
endif()

# set definitions and sources for MIPS
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mipsel*" OR
	CMAKE_SYSTEM_PROCESSOR MATCHES "mips64el*" )
  set(PNG_MIPS_MSA_POSSIBLE_VALUES on off)
  set(PNG_MIPS_MSA "on" CACHE STRING "Enable MIPS_MSA optimizations:
     off: disable the optimizations")
  set_property(CACHE PNG_MIPS_MSA PROPERTY STRINGS
     ${PNG_MIPS_MSA_POSSIBLE_VALUES})
  list(FIND PNG_MIPS_MSA_POSSIBLE_VALUES ${PNG_MIPS_MSA} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_MIPS_MSA must be one of [${PNG_MIPS_MSA_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_MIPS_MSA} STREQUAL "no")
    set(libpng_mips_sources
      mips/mips_init.c
      mips/filter_msa_intrinsics.c)
    if(${PNG_MIPS_MSA} STREQUAL "on")
      add_definitions(-DPNG_MIPS_MSA_OPT=2)
    endif()
  else()
    add_definitions(-DPNG_MIPS_MSA_OPT=0)
  endif()
endif()
endif(PNG_HARDWARE_OPTIMIZATIONS)

# SET LIBNAME
set(PNG_LIB_NAME png${PNGLIB_MAJOR}${PNGLIB_MINOR})

# to distinguish between debug and release lib
set(CMAKE_DEBUG_POSTFIX "d")

include(CheckCSourceCompiles)
option(ld-version-script "Enable linker version script" ON)
if(ld-version-script AND NOT APPLE)
  # Check if LD supports linker scripts.
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map" "VERS_1 {
        global: sym;
        local: *;
};

VERS_2 {
        global: sym2;
                main;
} VERS_1;
")
  set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} "-Wl,--version-script='${CMAKE_CURRENT_BINARY_DIR}/conftest.map'")
  check_c_source_compiles("void sym(void) {}
void sym2(void) {}
int main(void) {return 0;}
" HAVE_LD_VERSION_SCRIPT)
  if(NOT HAVE_LD_VERSION_SCRIPT)
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE} "-Wl,-M -Wl,${CMAKE_CURRENT_BINARY_DIR}/conftest.map")
  check_c_source_compiles("void sym(void) {}
void sym2(void) {}
int main(void) {return 0;}
" HAVE_SOLARIS_LD_VERSION_SCRIPT)
  endif()
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})
  file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map")
endif()

# Find symbol prefix.  Likely obsolete and unnecessary with recent
# toolchains (it's not done in many other projects).
function(symbol_prefix)
  set(SYMBOL_PREFIX)

  execute_process(COMMAND "${CMAKE_C_COMPILER}" "-E" "-"
                  INPUT_FILE /dev/null
                  OUTPUT_VARIABLE OUT
                  RESULT_VARIABLE STATUS)

  if(CPP_FAIL)
    message(WARNING "Failed to run the C preprocessor")
  endif()

  string(REPLACE "\n" ";" OUT "${OUT}")
  foreach(line ${OUT})
    string(REGEX MATCH "^PREFIX=" found_match "${line}")
    if(found_match)
      STRING(REGEX REPLACE "^PREFIX=(.*\)" "\\1" prefix "${line}")
      string(REGEX MATCH "__USER_LABEL_PREFIX__" found_match "${prefix}")
      if(found_match)
        STRING(REGEX REPLACE "(.*)__USER_LABEL_PREFIX__(.*)" "\\1\\2" prefix "${prefix}")
      endif()
      set(SYMBOL_PREFIX "${prefix}")
    endif()
  endforeach()

    message(STATUS "Symbol prefix: ${SYMBOL_PREFIX}")
    set(SYMBOL_PREFIX "${SYMBOL_PREFIX}" PARENT_SCOPE)
endfunction()

if(UNIX)
  symbol_prefix()
endif()

find_program(AWK NAMES gawk awk)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# No awk available to generate sources; use pre-built pnglibconf.h
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt
               ${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h)
add_custom_target(genfiles) # Dummy

# OUR SOURCES
set(libpng_public_hdrs
  png.h
  pngconf.h
  "${CMAKE_CURRENT_BINARY_DIR}/pnglibconf.h"
)
set(libpng_private_hdrs
  pngpriv.h
  pngdebug.h
  pnginfo.h
  pngstruct.h
)
set(libpng_sources
  ${libpng_public_hdrs}
  ${libpng_private_hdrs}
  png.c
  pngerror.c
  pngget.c
  pngmem.c
  pngpread.c
  pngread.c
  pngrio.c
  pngrtran.c
  pngrutil.c
  pngset.c
  pngtrans.c
  pngwio.c
  pngwrite.c
  pngwtran.c
  pngwutil.c
  ${libpng_arm_sources}
  ${libpng_intel_sources}
  ${libpng_mips_sources}
  ${libpng_powerpc_sources}
)
set(pngfix_sources
  contrib/tools/pngfix.c
)
set(png_fix_itxt_sources
  contrib/tools/png-fix-itxt.c
)

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

if(PNG_DEBUG)
  add_definitions(-DPNG_DEBUG)
endif()

# NOW BUILD OUR TARGET
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${ZLIB_INCLUDE_DIR})

unset(PNG_LIB_TARGETS)

if(PNG_SHARED)
  add_library(png SHARED ${libpng_sources})
  set(PNG_LIB_TARGETS png)
  set_target_properties(png PROPERTIES OUTPUT_NAME ${PNG_LIB_NAME})
  add_dependencies(png genfiles)
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(png PROPERTIES PREFIX "lib")
    set_target_properties(png PROPERTIES IMPORT_PREFIX "lib")
  endif()
  target_link_libraries(png ${ZLIB_LIBRARY} ${M_LIBRARY})

  if(UNIX AND AWK)
    if(HAVE_LD_VERSION_SCRIPT)
      set_target_properties(png PROPERTIES LINK_FLAGS
        "-Wl,--version-script='${CMAKE_CURRENT_BINARY_DIR}/libpng.vers'")
    elseif(HAVE_SOLARIS_LD_VERSION_SCRIPT)
      set_target_properties(png PROPERTIES LINK_FLAGS
        "-Wl,-M -Wl,'${CMAKE_CURRENT_BINARY_DIR}/libpng.vers'")
    endif()
  endif()
endif()

if(PNG_STATIC)
  # does not work without changing name
  set(PNG_LIB_NAME_STATIC png_static)
  add_library(png_static STATIC ${libpng_sources})
  add_dependencies(png_static genfiles)
  # MSVC doesn't use a different file extension for shared vs. static
  # libs.  We are able to change OUTPUT_NAME to remove the _static
  # for all other platforms.
  if(NOT MSVC)
    set_target_properties(png_static PROPERTIES
      OUTPUT_NAME "${PNG_LIB_NAME}"
      CLEAN_DIRECT_OUTPUT 1)
  else()
    set_target_properties(png_static PROPERTIES
      OUTPUT_NAME "${PNG_LIB_NAME}_static"
      CLEAN_DIRECT_OUTPUT 1)
  endif()
  list(APPEND PNG_LIB_TARGETS png_static)
  if(MSVC)
    # msvc does not append 'lib' - do it here to have consistent name
    set_target_properties(png_static PROPERTIES PREFIX "lib")
  endif()
  target_link_libraries(png_static ${ZLIB_LIBRARY} ${M_LIBRARY})
endif()

if(PNG_FRAMEWORK)
  set(PNG_LIB_NAME_FRAMEWORK png_framework)
  add_library(png_framework SHARED ${libpng_sources})
  add_dependencies(png_framework genfiles)
  list(APPEND PNG_LIB_TARGETS png_framework)
  set_target_properties(png_framework PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION ${PNGLIB_VERSION}
    MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PNGLIB_MAJOR}.${PNGLIB_MINOR}
    MACOSX_FRAMEWORK_BUNDLE_VERSION ${PNGLIB_VERSION}
    MACOSX_FRAMEWORK_IDENTIFIER org.libpng.libpng
    XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
    PUBLIC_HEADER "${libpng_public_hdrs}"
    OUTPUT_NAME png)
  target_link_libraries(png_framework ${ZLIB_LIBRARY} ${M_LIBRARY})
endif()

if(NOT PNG_LIB_TARGETS)
  message(SEND_ERROR
    "No library variant selected to build. "
    "Please enable at least one of the following options: "
    " PNG_STATIC, PNG_SHARED, PNG_FRAMEWORK")
endif()

if(PNG_SHARED AND WIN32)
  set_target_properties(png PROPERTIES DEFINE_SYMBOL PNG_BUILD_DLL)
endif()

if(PNG_SHARED)
  add_executable(pngfix ${pngfix_sources})
  target_link_libraries(pngfix png)
  set(PNG_BIN_TARGETS pngfix)

  add_executable(png-fix-itxt ${png_fix_itxt_sources})
  target_link_libraries(png-fix-itxt ${ZLIB_LIBRARY} ${M_LIBRARY})
  list(APPEND PNG_BIN_TARGETS png-fix-itxt)
endif()

# Set a variable with CMake code which:
# Creates a symlink from src to dest (if possible) or alternatively
# copies if different.
include(CMakeParseArguments)

function(CREATE_SYMLINK DEST_FILE)

  cmake_parse_arguments(S "" "FILE;TARGET" "" ${ARGN})

  if(NOT S_TARGET AND NOT S_FILE)
    message(FATAL_ERROR "Specify either a TARGET or a FILE for CREATE_SYMLINK to link to.")
  endif(NOT S_TARGET AND NOT S_FILE)

  if(S_TARGET AND S_FILE)
    message(FATAL_ERROR "CREATE_SYMLINK called with both source file ${S_FILE} and build target ${S_TARGET} arguments - can only handle 1 type per call.")
  endif(S_TARGET AND S_FILE)

  if(S_FILE)
    # If we don't need to symlink something that's coming from a build target,
    # we can go ahead and symlink/copy at configure time.

    if(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
      execute_process(
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${S_FILE} ${DEST_FILE}
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
    else(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
      execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${S_FILE} ${DEST_FILE}
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	)
    endif(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
  endif(S_FILE)

  if(S_TARGET)
    # We need to use generator expressions, which can be a bit tricky, so for
    # simplicity make the symlink a POST_BUILD step and use the TARGET
    # signature of add_custom_command.

    if(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
      add_custom_command(TARGET ${S_TARGET} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_LINKER_FILE_NAME:${S_TARGET}> $<TARGET_LINKER_FILE_DIR:${S_TARGET}>/${DEST_FILE}
	)
    else(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)
      add_custom_command(TARGET ${S_TARGET} POST_BUILD
	COMMAND "${CMAKE_COMMAND}" -E create_symlink $<TARGET_LINKER_FILE_NAME:${S_TARGET}> $<TARGET_LINKER_FILE_DIR:${S_TARGET}>/${DEST_FILE}
	)
    endif(CMAKE_HOST_WIN32 AND NOT CYGWIN AND NOT MSYS)

  endif(S_TARGET)

endfunction()

# Create source generation scripts.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/genchk.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/scripts/genchk.cmake @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/genout.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/scripts/genout.cmake @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/scripts/gensrc.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/scripts/gensrc.cmake @ONLY)


# libpng is a library so default to 'lib'
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

# CREATE PKGCONFIG FILES
# we use the same files like ./configure, so we have to set its vars
# Only do this on Windows for Cygwin - the files don't make much sense outside
# a UNIX look alike
if(NOT WIN32 OR CYGWIN OR MINGW)
  set(prefix      ${CMAKE_INSTALL_PREFIX})
  set(exec_prefix ${CMAKE_INSTALL_PREFIX})
  set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
  set(LIBS        "-lz -lm")
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc @ONLY)
  CREATE_SYMLINK(libpng.pc FILE ${PNGLIB_NAME}.pc)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libpng-config.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config @ONLY)
  CREATE_SYMLINK(libpng-config FILE ${PNGLIB_NAME}-config)
endif(NOT WIN32 OR CYGWIN OR MINGW)

# SET UP LINKS
if(PNG_SHARED)
  set_target_properties(png PROPERTIES
#   VERSION 16.${PNGLIB_RELEASE}.1.6.35
    VERSION 16.${PNGLIB_RELEASE}.0
    SOVERSION 16
    CLEAN_DIRECT_OUTPUT 1)
endif()

# INSTALL
if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL )
  install(TARGETS ${PNG_LIB_TARGETS}
      ${PNG_EXPORT_RULE}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if(PNG_SHARED)
    # Create a symlink for libpng.dll.a => libpng16.dll.a on Cygwin
    if(CYGWIN OR MINGW)
      CREATE_SYMLINK(libpng${CMAKE_IMPORT_LIBRARY_SUFFIX} TARGET png)
      install(FILES $<TARGET_LINKER_FILE_DIR:png>/libpng${CMAKE_IMPORT_LIBRARY_SUFFIX} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif(CYGWIN OR MINGW)

    if(NOT WIN32)
      CREATE_SYMLINK(libpng${CMAKE_SHARED_LIBRARY_SUFFIX} TARGET png)
      install(FILES $<TARGET_LINKER_FILE_DIR:png>/libpng${CMAKE_SHARED_LIBRARY_SUFFIX} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif(NOT WIN32)
  endif(PNG_SHARED)

  if(PNG_STATIC)
    if(NOT WIN32 OR CYGWIN OR MINGW)
      CREATE_SYMLINK( libpng${CMAKE_STATIC_LIBRARY_SUFFIX} TARGET png_static)
      install(FILES $<TARGET_LINKER_FILE_DIR:png_static>/libpng${CMAKE_STATIC_LIBRARY_SUFFIX} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif(NOT WIN32 OR CYGWIN OR MINGW)
 endif()
endif()

if(NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL )
  install(FILES ${libpng_public_hdrs}   DESTINATION include)
  install(FILES ${libpng_public_hdrs}   DESTINATION include/${PNGLIB_NAME})
endif()
if(NOT SKIP_INSTALL_EXECUTABLES AND NOT SKIP_INSTALL_ALL )
  if(NOT WIN32 OR CYGWIN OR MINGW)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config DESTINATION bin)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
            DESTINATION bin)
  endif(NOT WIN32 OR CYGWIN OR MINGW)
endif()

if(NOT SKIP_INSTALL_PROGRAMS AND NOT SKIP_INSTALL_ALL )
  install(TARGETS ${PNG_BIN_TARGETS}
      RUNTIME DESTINATION bin)
endif()

if(NOT SKIP_INSTALL_FILES AND NOT SKIP_INSTALL_ALL )
  # Install man pages
  if(NOT PNG_MAN_DIR)
    set(PNG_MAN_DIR "share/man")
  endif()
  install(FILES libpng.3 libpngpf.3      DESTINATION ${PNG_MAN_DIR}/man3)
  install(FILES png.5                    DESTINATION ${PNG_MAN_DIR}/man5)
  # Install pkg-config files
  if(NOT CMAKE_HOST_WIN32 OR CYGWIN OR MINGW)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpng.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/libpng-config
            DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PNGLIB_NAME}-config
            DESTINATION bin)
  endif(NOT CMAKE_HOST_WIN32 OR CYGWIN OR MINGW)
endif()

# On versions of CMake that support it, create an export file CMake
# users can include() to import our targets
if(PNG_EXPORT_RULE AND NOT SKIP_INSTALL_EXPORT AND NOT SKIP_INSTALL_ALL )
  install(EXPORT libpng DESTINATION lib/libpng FILE lib${PNG_LIB_NAME}.cmake)
endif()

# what's with libpng-manual.txt and all the extra files?

# UNINSTALL
# do we need this?

# DIST
# do we need this?

# to create msvc import lib for mingw compiled shared lib
# pexports libpng.dll > libpng.def
# lib /def:libpng.def /machine:x86
