function (register_lexer_test)

	if (NOT (ARGC EQUAL 1))
		set (MSG "register_lexer_test: Bad ARGC")
		message (FATAL_ERROR ${MSG})
	endif ()

	# Guess locations of source file, input file
	# and output file based on ARGV0
	set (FILE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/rf/${ARGV0}.rf)
	set (FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/in/${ARGV0}.in)
	set (FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/out/${ARGV0}.out)

	# Add test that internally reads from a file
	set (CMD lexer-test -f ${FILE_SRC} ${FILE_IN} ${FILE_OUT})
	add_test (NAME ${ARGV0}-file COMMAND ${CMD})

	# Add test that internally reads from a string
	set (CMD lexer-test -s ${FILE_SRC} ${FILE_IN} ${FILE_OUT})
	add_test (NAME ${ARGV0}-string COMMAND ${CMD})

endfunction ()


file(GLOB SOURCE_FILES src/*.c)
add_executable (lexer-test ${SOURCE_FILES})
target_include_directories (lexer-test PUBLIC ${INCLUDE_DIRS})
target_link_libraries (lexer-test common lexer)

# This tests TOK_END token
register_lexer_test (empty)

# This tests TOK_EOL token
register_lexer_test (newline)

# This tests TOK_DEC_* tokens
register_lexer_test (decimal)

# This tests TOK_BIN_* tokens
register_lexer_test (binary)
