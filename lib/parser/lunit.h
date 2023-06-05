#ifndef _LEXER_LUNIT_H_
#define _LEXER_LUNIT_H_

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
      An identifier
    */
    TOK_IDENTIFIER,
    /*
      Decimal constant with no errors detected 
    */
    TOK_DEC_INT,
    /*
      Decimal constant with dat at the end, eg. "10."
    */
    TOK_DEC_INT_DOT,
    /*
      Decimal constant with a comma at the end, eg. "12,345,"
      COM stands for comma
    */
    TOK_DEC_INT_COM,
    /*
      Decimal constant that has a dot or a comma
      followed by another dot or a comma, eg. "123..1"
      SEQ stands for sequence
    */
    TOK_DEC_INT_SEQ,
    /*
      A decimal constant followed by invalid characters, eg. "0xabcdefg"
      Here 'g' is the suffix, SUF stands for suffix
    */
    TOK_DEC_INT_SUF,
    /*
      A decimal floating point number
    */
    TOK_DEC_FLT,
    /*
      End of line and end of file
    */
    TOK_EOL,
    TOK_EOF,
    /*
      Unknown token, or in other words everything else
    */
    TOK_UNKNOWN
};

/* next field is set to NULL by lexer; it is used by parser */
struct lunit
{
    struct lunit *next;
    struct lstring *lexme;
    size_t line;
    size_t column;
    enum token token;
};

#endif
