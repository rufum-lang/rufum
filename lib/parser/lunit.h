#ifndef RUFUM_PARSER_LUNIT_H
#define RUFUM_PARSER_LUNIT_H

#include <stddef.h>

#include <common/lstring.h>

enum token
{
    /* Procedure declarations */
    //TOK_PROCEDURE,
    /* Argument and variable declarations */
    //TOK_INPUT,
    //TOK_DECLARE,
    //TOK_TYPE,
    /* If statement */
    //TOK_IF,
    //TOK_ELSE,
    /* While loop */
    //TOK_WHILE,
    //TOK_CONTINUE,
    //TOK_BREAK,
    /* Sections */
    //TOK_SECTION,
    //TOK_REPEAT,
    //TOK_SKIP,
    /* Match stetement */
    //TOK_MATCH,
    //TOK_WHEN,
    /* Arithmetical operators */
    //TOK_ADD,
    //TOK_SUB,
    //TOK_MUL,
    //TOK_DIV,
    /* Logical operators */
    //TOK_AND,
    //TOK_OR
    /* Bitwise operators */
    //TOK_BITAND,
    //TOK_BITOR,
    //TOK_BITXOR,
    /* Relational operators */
    //TOK_TEST_E,
    //TOK_TEST_NE,
    //TOK_TEST_G,
    //TOK_TEST_GE,
    //TOK_TEST_L,
    //TOK_TEST_LE,
    /* Records */
    //TOK_RECORD,
    //TOK_MEMBER,
    /* Return keyword */
    //TOK_RETURN,
    /*
      A lowercase identifier
    */
    TOK_LOWERCASE,
    /*
      Stray prefixes for binary, octal and hexadecimal numbers respectively
    */
    TOK_BIN_PREFIX,
    TOK_OCT_PREFIX,
    TOK_HEX_PREFIX,
    /*
      Integers with no errors detected 
    */
    TOK_BIN_INT,
    TOK_OCT_INT,
    TOK_DEC_INT,
    TOK_HEX_INT,
    /*
      Integer constant with dot at the end, eg. "10."
    */
    TOK_BIN_INT_DOT,
    TOK_OCT_INT_DOT,
    TOK_DEC_INT_DOT,
    TOK_HEX_INT_DOT,
    /*
      Integer constant with a comma at the end, eg. "12,345,"
      COM stands for comma
    */
    TOK_BIN_INT_COM,
    TOK_OCT_INT_COM,
    TOK_DEC_INT_COM,
    TOK_HEX_INT_COM,
    /*
      Integer constant that has a dot or a comma
      followed by another dot or a comma, eg. "123..1"
      SEQ stands for sequence
    */
    TOK_BIN_INT_SEQ,
    TOK_OCT_INT_SEQ,
    TOK_DEC_INT_SEQ,
    TOK_HEX_INT_SEQ,
    /*
      Integer constant followed by invalid characters, eg. "123g"
      Here 'g' is the suffix, SUF stands for suffix
    */
    TOK_BIN_INT_SUF,
    TOK_OCT_INT_SUF,
    TOK_DEC_INT_SUF,
    TOK_HEX_INT_SUF,
    /*
      Floating point constant with no errors detected
    */
    TOK_BIN_FLT,
    TOK_OCT_FLT,
    TOK_DEC_FLT,
    TOK_HEX_FLT,
    /*
      Floating point constant with two or more dots, eg. "127.0.0.1"
    */
    TOK_BIN_FLT_DOT,
    TOK_OCT_FLT_DOT,
    TOK_DEC_FLT_DOT,
    TOK_HEX_FLT_DOT,
    /*
      Floating point constant with a comma at the end, eg. "12.345,"
      COM stands for comma
    */
    TOK_BIN_FLT_COM,
    TOK_OCT_FLT_COM,
    TOK_DEC_FLT_COM,
    TOK_HEX_FLT_COM,
    /*
      Floating point constant that has a dot or a comma
      followed by another dot or a comma, eg. "0.123.,1"
      SEQ stands for sequence
    */
    TOK_BIN_FLT_SEQ,
    TOK_OCT_FLT_SEQ,
    TOK_DEC_FLT_SEQ,
    TOK_HEX_FLT_SEQ,
    /*
      Floating point constant followed by invalid characters, 
      Example: "0x0.abcdefg", Here 'g' is the suffix, SUF stands for suffix
    */
    TOK_BIN_FLT_SUF,
    TOK_OCT_FLT_SUF,
    TOK_DEC_FLT_SUF,
    TOK_HEX_FLT_SUF,
    /*
      End of line and end of source
    */
    TOK_EOL,
    TOK_END,
    /*
      One-line comment followed by end of input
    */
    TOK_BAD_COM,
    /*
      Unterminated multiline comment 
    */
    TOK_BAD_ML_COM,
    /*
      Unknown token, or in other words everything else
    */
    TOK_UNKNOWN
};

typedef struct lunit lunit_t;

/* next field is set to NULL by lexer; it is used by parser */
struct lunit
{
    lunit_t *next;
    lstring_t *lexme;
    size_t line;
    size_t column;
    enum token token;
};

#endif
