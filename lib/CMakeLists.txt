cmake_minimum_required (VERSION 3.10)

project (rufum VERSION 0.0.0)

# Description for RUFUM_BITS option
# It's long and this is why it's here
set (BITS_DESC "Configure build for 32-bit or 64-bit machine,")
set (BITS_DESC "${BITS_DESC} valid values: 32, 64 (default)")

option (RUFUM_BUILD_MODULE "Build as module" OFF)
option (RUFUM_BUILD_SHARED "Build shared library" OFF)
option (RUFUM_BUILD_STATIC "Build static library" OFF)
option (RUFUM_BITS ${BITS_DESC} 64)

# Check if we need to build anything
set (BUILD_NOTHING NOT ${RUFUM_BUILD_SHARED})
set (BUILD_NOTHING ${BUILD_NOTHING} AND NOT ${RUFUM_BUILD_SHARED})
set (BUILD_NOTHING ${BUILD_NOTHING} AND NOT ${RUFUM_BUILD_STATIC})

# Nothing to do? Print an error message
if (${BUILD_NOTHING})
	set (MSG "At least one option of RUFUM_BUILD_MODULE,")
	set (MSG "${MSG} RUFUM_BUILD_SHARED, RUFUM_BUILD_STATIC")
	set (MSG "${MSG} must be specified")
	message (FATAL_ERROR ${MSG})
endif ()

# Variables RUFUM_SIGNED and RUFUM_UNSIGNED are needed to
# define mete_signed_t and mete_unsigned_t respectively
# They are used to configure $BUILD/external/config.h
# This file will contain typedef ${RUFUM_SIGNED} mete_signed_t
# and typedef ${RUFUM_UNSIGNED} mete_unsigned_t
# If METE_BITS is equal neither 32 nor 64 print an error message
if (${RUFUM_BITS} EQUAL 32)
	set (RUFUM_SIGNED int_least32_t)
	set (RUFUM_UNSIGNED uint_least32_t)
	set (RUFUM_SIGNED_MIN INT_LEAST32_MIN)
	set (RUFUM_SIGNED_MAX INT_LEAST32_MAX)
	set (RUFUM_UNSIGNED_MAX UINT_LEAST32_MAX)
elseif (${RUFUM_BITS} EQUAL 64)
	set (RUFUM_SIGNED int_least64_t)
	set (RUFUM_UNSIGNED uint_least64_t)
	set (RUFUM_SIGNED_MIN INT_LEAST64_MIN)
	set (RUFUM_SIGNED_MAX INT_LEAST64_MAX)
	set (RUFUM_UNSIGNED_MAX UINT_LEAST64_MAX)
else ()
	set (MSG "Option RUFU_BITS must be one of 32, 64 (default)")
	message (FATAL_ERROR ${MSG})
endif ()

# Collect information about all the .c files located in
# directory CMakeLists.txt resides in and its subdirectories
file (GLOB_RECURSE SOURCE_FILES *.c)
file (GLOB HEADER_FILES headers/*.h)

# EXTERNAL_DIR is for header files that we are going to install
# INTERNAL_DIR is for one header file that needs configuration
# INCLUDE_DIR is where we are looking for include files
set (EXTERNAL_DIR ${CMAKE_BINARY_DIR}/external)
set (INTERNAL_DIR ${CMAKE_BINARY_DIR}/internal)
set (INCLUDE_DIRS ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

# Make directories for header files
file (MAKE_DIRECTORY ${EXTERNAL_DIR})
file (MAKE_DIRECTORY ${INTERNAL_DIR})

# Why copy these header files? Here is the reason:
# $SRC/headers/mete.h needs to include $BUILD/external/config.h
# Once we install them they will be installed in one directory
# This means that in mete.h we #include "config.h"
# TODO Do we really need to include mete.h?
file (COPY ${HEADER_FILES} DESTINATION ${EXTERNAL_DIR})


configure_file (config/external.h.in ${EXTERNAL_DIR}/config.h)
configure_file (config/internal.h.in ${INTERNAL_DIR}/config.h)

if (${RUFUM_BUILD_MODULE})
	add_library (rufum-module MODULE ${SOURCE_FILES})
	target_include_directories (rufum-module PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-module PROPERTIES OUTPUT_NAME rufum)
endif ()

if (${RUFUM_BUILD_SHARED})
	add_library (rufum-shared SHARED ${SOURCE_FILES})
	target_include_directories (rufum-shared PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-shared PROPERTIES OUTPUT_NAME rufum)
endif ()

if (${RUFUM_BUILD_STATIC})
	add_library (rufum-static STATIC ${SOURCE_FILES})
	target_include_directories (rufum-static PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-static PROPERTIES OUTPUT_NAME rufum)
endif ()
