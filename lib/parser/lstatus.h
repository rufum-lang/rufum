#ifndef RUFUM_LEXER_LERROR_H
#define RUFUM_LEXER_LERROR_H

enum lexer_status
{
    LEXER_OK,
    LEXER_MEMORY_ERROR,
    LEXER_IO_ERROR,
    LEXER_LINE_LIMIT_ERROR,
    LEXER_COLUMN_LIMIT_ERROR,
    LEXER_BAD_COMMENT,
    LEXER_BAD_MULTILINE_COMMENT
};

typedef enum lexer_status lstatus_t;

#endif
