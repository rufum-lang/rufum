cmake_minimum_required (VERSION 3.10)

project (rufum VERSION 0.0.0 LANGUAGES C)

# We are using C99 with extensions disabled
set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED ON)
set (CMAKE_C_EXTENSIONS OFF)

# Descriptions for some options
# They are long and this is why thay are here
set (EXECUTABLE_DESC "Build interpreter, implies RUFUM_BUILD_STATIC")

set (BITS_DESC "Configure build for 32-bit or 64-bit machine,")
set (BITS_DESC "${BITS_DESC} valid values: 32, 64 (default)")

# Options that can be specified at command line
option (RUFUM_BUILD_MODULE "Build module library" OFF)
option (RUFUM_BUILD_SHARED "Build shared library" OFF)
option (RUFUM_BUILD_STATIC "Build static library" OFF)
option (RUFUM_BUILD_EXECUTABLE ${EXECUTABLE_DESC} OFF)
option (RUFUM_ENABLE_TESTS "Enable tests" OFF)
set (RUFUM_BITS 64 CACHE STRING ${BITS_DESC})

# Check if we need to build anything
set (BUILD_NOTHING NOT ${RUFUM_BUILD_SHARED})
set (BUILD_NOTHING ${BUILD_NOTHING} AND NOT ${RUFUM_BUILD_SHARED})
set (BUILD_NOTHING ${BUILD_NOTHING} AND NOT ${RUFUM_BUILD_STATIC})
set (BUILD_NOTHING ${BUILD_NOTHING} AND NOT ${RUFUM_BUILD_EXECUTABLE})

# Nothing to do? Print an error message
if (${BUILD_NOTHING} AND NOT ${RUFUM_ENABLE_TESTS})
	set (MSG "At least one option of RUFUM_BUILD_MODULE,")
	set (MSG "${MSG} RUFUM_BUILD_SHARED, RUFUM_BUILD_STATIC,")
	set (MSG "${MSG} RUFUM_BUILD_EXECUTABLE must be used")
	message (FATAL_ERROR ${MSG})
endif ()

# Only build tests? Notify user about it
if (${BUILD_NOTHING} AND ${RUFUM_ENABLE_TESTS})
	set (MSG "With current configuration only tests are going to be built")
	message (STATUS ${MSG})
endif ()

# This is a special case when we build static library
# to build the interpreter but do not install the library
if (${RUFUM_BUILD_EXECUTABLE} AND NOT ${RUFUM_BUILD_STATIC})
	set (RUFUM_BUILD_STATIC ON)
	set (STATIC_LIB_NO_INSTALL ON)
endif ()

# Variables RUFUM_SIGNED and RUFUM_UNSIGNED are needed to
# define rufum_signed_t and rufum_unsigned_t respectively
# They are used to configure $BUILD/external/config.h
# This file will contain typedef ${RUFUM_SIGNED} mete_signed_t
# and typedef ${RUFUM_UNSIGNED} mete_unsigned_t
# If METE_BITS is equal to neither 32 nor 64 print an error message
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
	set (MSG "Option RUFUM_BITS must be one of 32, 64 (default)")
	message (STATUS ${RUFUM_BITS})
	message (FATAL_ERROR ${MSG})
endif ()

# We need to enable testing in root CMakeLists.txt
# Otherwise make test won't be available in root directory
if (${RUFUM_ENABLE_TESTS})
	enable_testing ()
endif ()

# EXTERNAL_DIR is for header files that we are going to install
# INTERNAL_DIR is for one header file that needs configuration
set (EXTERNAL_DIR ${CMAKE_BINARY_DIR}/external)
set (INTERNAL_DIR ${CMAKE_BINARY_DIR}/internal)

# Make directories for header files
file (MAKE_DIRECTORY ${EXTERNAL_DIR})
file (MAKE_DIRECTORY ${INTERNAL_DIR})

# Why copy these header files? Here is the reason:
# $SRC/headers/rufum.h needs to include $BUILD/external/config.h
# Once we install them they will be installed in one directory
# This means that in rufum.h we #include "config.h"
# TODO Do we really need to include rufum.h?
file (COPY ${HEADER_FILES} DESTINATION ${EXTERNAL_DIR})


configure_file (config/rufum.h.in ${EXTERNAL_DIR}/rufum.h)
configure_file (config/internal.h.in ${INTERNAL_DIR}/config.h)

# We have already checked if there is something to build
# If we aren't building the interpreter
# then we are building the library 
# If we are building the interpreter
# then we are building the library also
# Unconditionally build the library
add_subdirectory (lib)


if (${RUFUM_BUILD_EXECUTABLE})
	# There is no bin directory as of now
	# add_subdirectory (bin)
endif ()

