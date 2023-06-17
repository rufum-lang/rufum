
file(GLOB SOURCE_FILES src/*.c)
add_executable (lexer-test ${SOURCE_FILES})
target_include_directories (lexer-test PUBLIC ${INCLUDE_DIRS})
target_link_libraries (lexer-test common lexer)
