# INCLUDE_DIR is where we are looking for include files

set (INCLUDE_DIRS ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# This function registers a target that will build regular (non-PIC)
# object file from all .c files located in a directory and its subdirectories
# It takes only one argument - derectory name

function (register_object)

	# Check if we were invoked with right number of arguments

	if (NOT (${ARGC} EQUAL 1))
		message (FATAL_ERROR "register_object: Bad ARGC")
	endif ()

	# Collect information about all the .c files located in
	# directory ${ARGV0} and its subdirectories
	# Files with extension .i.c aren't meant to be compiled
	# but to be included in other C files
	# They aren't header files, they contain actual C code
	# They use .c extension for syntax highlighting

	file (GLOB_RECURSE SOURCE_FILES ${ARGV0}/*.c)
	list (FILTER SOURCE_FILES EXCLUDE REGEX ".i.c$")

	# Register a ${DIRECTORY_NAME} target
	# Do not compile it by default (EXCLUDE_FROM_ALL)

	add_library (${ARGV0} OBJECT EXCLUDE_FROM_ALL ${SOURCE_FILES})
	target_include_directories (${ARGV0} PUBLIC ${INCLUDE_DIRS})

endfunction ()

# This function registers a target that will build a position-indepentent
# object file from all .c files located in a directory and its subdirectories
# It takes only one argument - directory name

function (register_object_pic)

	# Check if we were invoked with right number of arguments

	if (NOT (${ARGC} EQUAL 1))
		message (FATAL_ERROR "register_object_pic: Bad ARGC")
	endif ()

	# Collect information about all the .c files located in
	# directory ${ARGV0} and its subdirectories
	# Files with extension .i.c aren't meant to be compiled
	# but to be included in other C files
	# They aren't header files, they contain actual C code
	# They use .c extension for syntax highlighting

	file (GLOB_RECURSE SOURCE_FILES ${ARGV0}/*.c)
	list (FILTER SOURCE_FILES EXCLUDE REGEX ".i.c$")

	# Register a ${DIRECTORY_NAME}-pic target
	# Do not compile it by default (EXCLUDE_FROM_ALL)

	add_library (${ARGV0}-pic OBJECT EXCLUDE_FROM_ALL ${SOURCE_FILES})
	target_include_directories (${ARGV0}-pic PUBLIC ${INCLUDE_DIRS})

endfunction ()

set (DIRS common lexer)

foreach (OBJ ${DIRS})
	register_object (${OBJ})
	register_object_pic (${OBJ})
endforeach ()

# This variable is a list of object files static library depend on

set (OBJS ${DIRS})

# This is a list of object files shared and module libraries depend on

set (PIC_OBJS ${DIRS})
list (TRANSFORM PIC_OBJS APPEND -pic)


# Apparantly some build systems (such as XCode) don't like targets 
# that do not compile any source files and just link object files
# We are going to compile this empty file as a workaround

set (DUMMY_FILE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)

file (WRITE ${DUMMY_FILE} "")

# Variables RUFUM_BUILD_* and STATIC_LIB_NO_INSTALL
# come from parent CMakeLists.txt

if (${RUFUM_BUILD_MODULE})

	add_library (rufum-module MODULE dummy.c)
	add_dependencies (rufum-module ${PIC_OBJS})
	target_include_directories (rufum-module PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-module PROPERTIES OUTPUT_NAME rufum)
	install (TARGETS rufum-module)

endif ()

if (${RUFUM_BUILD_SHARED})

	add_library (rufum-shared SHARED dummy.c)
	add_dependencies (rufum-shared ${PIC_OBJS})
	target_include_directories (rufum-shared PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-shared PROPERTIES OUTPUT_NAME rufum)
	install (TARGETS rufum-shared)

endif ()

if (${RUFUM_BUILD_STATIC})

	add_library (rufum-static STATIC dummy.c)
	add_dependencies (rufum-static ${OBJS})
	target_include_directories (rufum-static PUBLIC ${INCLUDE_DIRS})
	set_target_properties (rufum-static PROPERTIES OUTPUT_NAME rufum)

	# It is possible that we build static library because interpreter
	# depends on it, in this case we do not want to install it
	# However if user explictly told us to build it we build it anyway

	if (NOT ${STATIC_LIB_NO_INSTALL})
		install (TARGETS rufum-static)
	endif ()

endif ()

add_subdirectory (tests)
