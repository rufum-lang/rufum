# INCLUDE_DIR is where we are looking for include files
set (INCLUDE_DIRS ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# Collect information about all the .c files located in
# directory this CMakeLists.txt resides in and its subdirectories
file (GLOB_RECURSE SOURCE_FILES *.c)

# Files with extension .i.c aren't meant to be compiled
# but to be included in other C files
# They aren't header files, they contain actual C code
# They use .c extension for syntax highlighting
list (FILTER SOURCE_FILES EXCLUDE REGEX ".i.c$")

# Header files to be installed
file (GLOB HEADER_FILES headers/*.h)

# Variables RUFUM_BUILD_* and STATIC_LIB_NO_INSTALL
# come from parent CMakeLists.txt

if (${RUFUM_BUILD_MODULE})
	add_library (rufum-module MODULE ${SOURCE_FILES})
	target_include_directories (rufum-module PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-module PROPERTIES OUTPUT_NAME rufum)
	install (TARGETS rufum-module)
endif ()

if (${RUFUM_BUILD_SHARED})
	add_library (rufum-shared SHARED ${SOURCE_FILES})
	target_include_directories (rufum-shared PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-shared PROPERTIES OUTPUT_NAME rufum)
	install (TARGETS rufum-shared)
endif ()

if (${RUFUM_BUILD_STATIC})
	add_library (rufum-static STATIC ${SOURCE_FILES})
	target_include_directories (rufum-static PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-static PROPERTIES OUTPUT_NAME rufum)

	# It is possible that we build static library because interpreter
	# depends on it, in this case we do not want to install it
	# However if user explictly told us to build it we build it anyway
	if (NOT ${STATIC_LIB_NO_INSTALL})
		install (TARGETS rufum-static)
	endif ()
endif ()
