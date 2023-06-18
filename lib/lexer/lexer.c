#include "lstatus.h"
#include "lunit.h"
#include "source.h"

#include <common/lstring.h>

#include <stdlib.h>

/*
  This function returns state_fn but you can't write that in C
  Hence we return is as void *
*/
typedef void *(*state_fn)(token_t *token, int c);

typedef struct lexme_info lexme_info_t;


struct lexme_info
{
    char *text;
    size_t line;
    size_t column;
    size_t index;
    size_t space;
};

/*
  This file (lexer.c) contains finite state machine functions only
  All other functions were split to separate files
  They are declared as static and only included here
  They are as follows:
  - categories.i.c - test_char_* functions used to test if character
    belongs to a certain category of characters
  - skip.i.c - functions used to skip spaces, comments and escaped newlines
    main function is called skip
  - util.i.c - other functions, namely lexme_append, lexme_empty,
    handle_error, handle_skip_error and create_lunit
*/

#include "categories.i.c"
#include "skip.i.c"
#include "util.i.c"

/*
  IDENTIFIER STATES
  =================
*/
static void *state_lowercase(token_t *token, int c)
{
    /*
      This function deals with lowercase idetifiers: they can be
      a variable name, a procedure name or a section name
      Function test_char_following checks if character can form an identifier
      While first character was a lowercase letter following characters
      can be any of a-z A-Z 0-9 _ ?
      This transition checks if we are dealing with following character
    */
    if (test_char_following(c))
        return &state_lowercase;

    /*
      We encountered some other character. Example:
        previous characters: l o w e r c a s e
        current character: ;
      That character is going to be unread because we are returning NULL
    */
    *token = TOK_LOWERCASE;

    return NULL;
}

static void *state_uppercase(token_t *token , int c)
{
    /*
      This function deals with uppercase idetifiers: they can be
      a type name, a trait name or a module name
      Function test_char_following checks if character can form an identifier
      While first character was a uppercase letter following characters
      can be any of a-z A-Z 0-9 _ ?
      This transition checks if we are dealing with following character
    */
    if (test_char_following(c))
        return &state_uppercase;

    /*
      We encountered some other character. Example:
        previous characters: U p p e r c a s e
        current character: #
      That character is going to be unread because we are returning NULL
    */
    *token = TOK_UPPERCASE;

    return NULL;
}

/*
  DECIMAL FLOATING POINT STATES
  =============================

  This declaration is needed because state_decimal_float
  calls state_decimal_float_comma and vice versa
*/
static void *state_decimal_float(token_t *token, int c);


static void *state_decimal_float_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in decimal system
      That illegal character wasn't a second dot as this case
      is handled by state_decimal_float_dot function
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_decimal_float_sequence function
      We have read a character that can form an identifier
      but can't form a decimal floating point number
      That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_decimal_float_suffix;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_DEC_FLT_SUF;

    return NULL;
}

static void *state_decimal_float_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_decimal_float_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_DEC_FLT_SEQ;

    return NULL;
}

static void *state_decimal_float_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      One of them was a dot. However now we have read another dot
      Number with two decimal points is an illegel construct
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_decimal_float_dot;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_DEC_FLT_DOT;

    return NULL;
}

static void *state_decimal_float_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a decimal digit then we have a valid number
    */
    if (test_char_decimal(c))
        return &state_decimal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_decimal_float_sequence;

    /*
      If it is a valid identifier character but not a valid decimal digit
      then we have a decimal floating point number with an invalid suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a number terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_DEC_FLT_COM;

    return NULL;
}

static void *state_decimal_float(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that th last one was a decimal digit
      What we do depends on next character read

      If it is a decimal digit then we have a valid number
      We make a transition to this state
    */
    if (test_char_decimal(c))
        return &state_decimal_float;

    /*
      If it is a decimal point then the number is invalid since we are
      dealing with a float which means we have already read a dot
    */
    if (c == '.')
        return &state_decimal_float_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_decimal_float_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_decimal_float_comma;

    /*
      If it is a valid identifier character but invalid number character
      then we have a decimal floating point number with invalid suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a valid number
      Report this finding to the caller
    */
    *token = TOK_DEC_FLT;

    return NULL;
}

/*
  BINARY FLOATING POINT STATES
  ============================

  This declaration is needed because state_binary_float
  calls state_binary_float_comma and vice versa
*/
static void *state_binary_float(token_t *token, int c);


static void *state_binary_float_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in binary system
      That illegal character wasn't a second dot as this case
      is handled by state_binary_float_dot function
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_binary_float_sequence function
      We have read a character that can form an identifier
      but can't form a binary floating point number
      That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_binary_float_suffix;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_BIN_FLT_SUF;

    return NULL;
}

static void *state_binary_float_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_binary_float_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_BIN_FLT_SEQ;

    return NULL;
}

static void *state_binary_float_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      One of them was a dot. However now we have read another dot
      Number with two decimal points is an illegel construct
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_binary_float_dot;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_BIN_FLT_DOT;

    return NULL;
}

static void *state_binary_float_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a binary digit then we have a valid number
    */
    if (test_char_binary(c))
        return &state_binary_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_binary_float_sequence;

    /*
      If it is a valid identifier character but not a valid decimal digit
      then we have a binary floating point number with an invalid suffix
    */
    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a number terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_BIN_FLT_COM;

    return NULL;
}

static void *state_binary_float(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that th last one was a binary digit
      What we do depends on next character read

      If it is a binary digit then we have a valid number
      We make a transition to this state
    */
    if (test_char_binary(c))
        return &state_binary_float;

    /*
      If it is a decimal point then the number is invalid since we are
      dealing with a float which means we have already read a dot
    */
    if (c == '.')
        return &state_binary_float_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_binary_float_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_binary_float_comma;

    /*
      If it is a valid identifier character but invalid number character
      then we have a binary floating point number with invalid suffix
    */
    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a valid number
      Report this finding to the caller
    */
    *token = TOK_BIN_FLT;

    return NULL;
}

/*
  OCTAL FLOATING POINT STATES
  ===========================

  This declaration is needed because state_octal_float
  calls state_octal_float_comma and vice versa
*/
static void *state_octal_float(token_t *token, int c);


static void *state_octal_float_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in octal system
      That illegal character wasn't a second dot as this case
      is handled by state_octal_float_dot function
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_octal_float_sequence function
      We have read a character that can form an identifier
      but can't form a decimal floating point number
      That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_octal_float_suffix;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_OCT_FLT_SUF;

    return NULL;
}

static void *state_octal_float_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_octal_float_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_OCT_FLT_SEQ;

    return NULL;
}

static void *state_octal_float_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      One of them was a dot. However now we have read another dot
      Number with two decimal points is an illegel construct
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_octal_float_dot;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_OCT_FLT_DOT;

    return NULL;
}

static void *state_octal_float_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a octal digit then we have a valid number
    */
    if (test_char_octal(c))
        return &state_octal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_octal_float_sequence;

    /*
      If it is a valid identifier character but not a valid decimal digit
      then we have a octal floating point number with an invalid suffix
    */
    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a number terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_OCT_FLT_COM;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a valid number
      Report this finding to the caller
    */
    return NULL;
}

static void *state_octal_float(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that th last one was an octal digit
      What we do depends on next character read

      If it is an octal digit then we have a valid number
      We make a transition to this state
    */
    if (test_char_octal(c))
        return &state_octal_float;

    /*
      If it is a decimal point then the number is invalid since we are
      dealing with a float which means we have already read a dot
    */
    if (c == '.')
        return &state_octal_float_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_octal_float_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_octal_float_comma;

    /*
      If it is a valid identifier character but invalid number character
      then we have an octal floating point number with invalid suffix
    */
    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a valid number
      Report this finding to the caller
    */
    *token = TOK_OCT_FLT;

    return NULL;
}

/*
  HEXADECIMAL FLOATING POINT STATES
  =================================

  This declaration is needed because state_hexadecimal_float
  calls state_hexadecimal_float_comma and vice versa
*/
static void *state_hexadecimal_float(token_t *token, int c);


static void *state_hexadecimal_float_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in hexadecimal system
      That illegal character wasn't a second dot as this case
      is handled by state_hexadecimal_float_dot function
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_hexadecimal_float_sequence function
      We have read a character that can form an identifier
      but can't form a hexadecimal floating point number
      That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_hexadecimal_float_suffix;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_HEX_FLT_SUF;

    return NULL;
}

static void *state_hexadecimal_float_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_hexadecimal_float_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_HEX_FLT_SEQ;

    return NULL;
}

static void *state_hexadecimal_float_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      One of them was a dot. However now we have read another dot
      Number with two decimal points is an illegel construct
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_hexadecimal_float_dot;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_HEX_FLT_DOT;

    return NULL;
}

static void *state_hexadecimal_float_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a hexadecimal digit then we have a valid number
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_hexadecimal_float_sequence;

    /*
      If it is a valid identifier character but not a valid decimal digit
      then we have a hexadecimal floating point number with an invalid suffix
    */
    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a number terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_HEX_FLT_COM;

    return NULL;
}

static void *state_hexadecimal_float(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that th last one was a hexadecimal digit
      What we do depends on next character read

      If it is a hexadecimal digit then we have a valid number
      We make a transition to this state
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    /*
      If it is a decimal point then the number is invalid since we are
      dealing with a float which means we have already read a dot
    */
    if (c == '.')
        return &state_hexadecimal_float_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_hexadecimal_float_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_hexadecimal_float_comma;

    /*
      If it is a valid identifier character but invalid number character
      then we have a hexadecimal floating point number with invalid suffix
    */
    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have a valid number
      Report this finding to the caller
    */
    *token = TOK_HEX_FLT;

    return NULL;
}

/*
  DECIMAL INTEGER STATES
  ======================

  This declaration is needed because state_decimal_int
  calls state_decimal_int_comma and vice versa
*/
static void *state_decimal_int(token_t *token, int c);


static void *state_decimal_int_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in decimal system
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_decimal_int_sequence function
      We have read a character that can form an identifier but can't form
      a decimal integer. That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If a character can form neither an identifier nor a number
      then we have finished reading that invalid number
      Report to the caller that we have found a decimal integer
      with a suffix
    */
    *token = TOK_DEC_INT_SUF;

    return NULL;
}

static void *state_decimal_int_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_decimal_int_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_DEC_INT_SEQ;

    return NULL;
}

static void *state_decimal_int_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot. These characters were already appended
      to the lexme. What we do depends on next character read

      If it is a decimal digit then we have a valid integer
    */
    if (test_char_decimal(c))
        return &state_decimal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_decimal_int_sequence;

    /*
      If it is a valid identifier character but not a valid decimal
      digit then we have a decimal integer with an invalid suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a dot
      Report this finding to the caller
    */
    *token = TOK_DEC_INT_DOT;

    return NULL;
}

static void *state_decimal_int_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a decimal digit then we have a valid number
    */
    if (test_char_decimal(c))
        return &state_decimal_int;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_decimal_int_sequence;

    /*
      If it is a valid identifier character but not a valid decimal
      digit then we have a decimal integer with an invalid suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_DEC_INT_COM;

    return NULL;
}

static void *state_decimal_int(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a decimal digit or a comma
      What we do depends on next character read

      If it is a decimal digit then we make a transition to this state
    */
    if (test_char_decimal(c))
        return &state_decimal_int;

    /*
      If it is a dot then we make this transition to next state
      It is up to it to decide whether it is a float or an invalid number
    */
    if (c == '.')
        return &state_decimal_int_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_decimal_int_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_decimal_int_comma;

    /*
      If it is a valid identifier character but invalid number
      character then we have a decimal integer with invalid suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If none of the transitions above was made then the character isn't
      part of the number. This means that this number is a valid integer
    */
    *token = TOK_DEC_INT;

    return NULL;
}

/*
  BINARY INTEGER STATES
  =====================

  This declaration is needed because state_binary_int
  calls state_binary_int_comma and vice versa
*/
static void *state_binary_int(token_t *token, int c);


static void *state_binary_int_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in binary system
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_binary_int_sequence function
      We have read a character that can form an identifier but can't form
      a binary integer. That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_binary_int_suffix;

    /*
      If a character can form neither an identifier nor a number
      then we have finished reading that invalid number
      Report to the caller that we have found a binary integer
      with a suffix
    */
    *token = TOK_BIN_INT_SUF;

    return NULL;
}

static void *state_binary_int_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_binary_int_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_BIN_INT_SEQ;

    return NULL;
}

static void *state_binary_int_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot. These characters were already appended
      to the lexme. What we do depends on next character read

      If it is a binary digit then we have a valid integer
    */
    if (test_char_binary(c))
        return &state_binary_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_binary_int_sequence;

    /*
      If it is a valid identifier character but not a valid binary
      character then we have a binary integer with an invalid suffix
    */
    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a dot
      Report this finding to the caller
    */
    *token = TOK_BIN_INT_DOT;

    return NULL;
}

static void *state_binary_int_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a binary digit then we have a valid number
    */
    if (test_char_binary(c))
        return &state_binary_int;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_binary_int_sequence;

    /*
      If it is a valid identifier character but not a valid binary
      digit then we have a binary integer with an invalid suffix
    */
    if (test_char_binary_suffix(c))
        return &state_binary_int_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_BIN_INT_COM;

    return NULL;
}

static void *state_binary_int(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a binary digit or a comma
      What we do depends on next character read

      If it is a binary digit then we make a transition to this state
    */
    if (test_char_binary(c))
        return &state_binary_int;

    /*
      If it is a dot then we make this transition to next state
      It is up to it to decide whether it is a float or an invalid number
    */
    if (c == '.')
        return &state_binary_int_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_binary_int_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_binary_int_comma;

    /*
      If it is a valid identifier character but invalid number
      character then we have a decimal integer with invalid suffix
    */
    if (test_char_binary_suffix(c))
        return &state_binary_int_suffix;

    /*
      If none of the transitions above was made then the character isn't
      part of the number. This means that this number is a valid integer
    */
    *token = TOK_BIN_INT;

    return NULL;
}

static void *state_binary_prefix(token_t *token, int c)
{
    /*
      So far we have read a '0b' suffix. If next character is
      a binary digit then we have a binary integer
    */
    if (test_char_binary(c))
        return &state_binary_int;

    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If none of the above transitions were made then we have 0 with suffix b
    */
    *token = TOK_DEC_INT_SUF;

    return NULL;
}

/*
  OCTAL INTEGER STATES
  ====================

  This declaration is needed because state_octal_int
  calls state_octal_int_comma and vice versa
*/
static void *state_octal_int(token_t *token, int c);


static void *state_octal_int_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in octal system
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_octal_int_sequence function
      We have read a character that can form an identifier but can't form
      an octal integer. That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_octal_int_suffix;

    /*
      If a character can form neither an identifier nor a number
      then we have finished reading that invalid number
      Report to the caller that we have found an octal integer
      with a suffix
    */
    *token = TOK_OCT_INT_SUF;

    return NULL;
}

static void *state_octal_int_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_octal_int_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_OCT_INT_SEQ;

    return NULL;
}

static void *state_octal_int_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot. These characters were already appended
      to the lexme. What we do depends on next character read

      If it is an octal digit then we have a valid integer
    */
    if (test_char_octal(c))
        return &state_octal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_octal_int_sequence;

    /*
      If it is a valid identifier character but not a valid octal
      digit then we have a octal integer with an invalid suffix
    */
    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a dot
      Report this finding to the caller
    */
    *token = TOK_OCT_INT_DOT;

    return NULL;
}

static void *state_octal_int_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is an octal digit then we have a valid number
    */
    if (test_char_octal(c))
        return &state_octal_int;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_octal_int_sequence;

    /*
      If it is a valid identifier character but not a valid octal
      digit then we have an octal integer with an invalid suffix
    */
    if (test_char_octal_suffix(c))
        return &state_octal_int_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_OCT_INT_COM;

    return NULL;
}

static void *state_octal_int(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was an octal digit or a comma
      What we do depends on next character read

      If it is an octal digit then we make a transition to this state
    */
    if (test_char_octal(c))
        return &state_octal_int;

    /*
      If it is a dot then we make this transition to next state
      It is up to it to decide whether it is a float or an invalid number
    */
    if (c == '.')
        return &state_octal_int_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_octal_int_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_octal_int_comma;

    /*
      If it is a valid identifier character but invalid number
      character then we have a decimal integer with invalid suffix
    */
    if (test_char_octal_suffix(c))
        return &state_octal_int_suffix;

    /*
      If none of the transitions above was made then the character isn't
      part of the number. This means that this number is a valid integer
    */
    *token = TOK_OCT_INT;

    return NULL;
}

static void *state_octal_prefix(token_t *token, int c)
{
    /*
      So far we have read a '0o' suffix. If next character is
      an octal digit then we have an octal integer
    */
    if (test_char_octal(c))
        return &state_octal_int;

    /*
      If this number is something else that can form a suffix
      then we have a decimal number 0 with a suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If none of the above transitions were made then we have 0 with suffix o
    */
    *token = TOK_DEC_INT_SUF;

    return NULL;
}

/*
  HEXADECIMAL INTEGER STATES
  ==========================

  This declaration is needed because state_hexadecimal_int
  calls state_hexadecimal_int_comma and vice versa
*/
static void *state_hexadecimal_int(token_t *token, int c);


static void *state_hexadecimal_int_suffix(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a character that is illegal in hexadecimal system
      It wasn't a dot or comma preceded by another dot or comma either
      This is handled by state_hexadecimal_int_sequence function
      We have read a character that can form an identifier but can't form
      an hexadecimal integer. That character was already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_hexadecimal_int_suffix;

    /*
      If a character can form neither an identifier nor a number
      then we have finished reading that invalid number
      Report to the caller that we have found a hexadecimal integer
      with a suffix
    */
    *token = TOK_HEX_INT_SUF;

    return NULL;
}

static void *state_hexadecimal_int_sequence(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot or comma followed by another dot or comma
      All these characters were already appended to the lexme
      Our task is to keep reading both legal and illegal characters until
      we encounter something that can't form an identifier nor a number
      Function test_char_suffix checks if given character can form
      a number or an identifier
    */
    if (test_char_suffix(c))
        return &state_hexadecimal_int_sequence;

    /*
      We encountered something that can't form a number nor an identidier
      Our job is done. That character will be unread
    */
    *token = TOK_HEX_INT_SEQ;

    return NULL;
}

static void *state_hexadecimal_int_dot(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      followed by a dot. These characters were already appended
      to the lexme. What we do depends on next character read

      If it is a hexadecimal digit then we have a valid integer
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_hexadecimal_int_sequence;

    /*
      If it is a valid identifier character but not a valid hexadecimal
      digit then we have a hexadecimal integer with an invalid suffix
    */
    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a dot
      Report this finding to the caller
    */
    *token = TOK_HEX_INT_DOT;

    return NULL;
}

static void *state_hexadecimal_int_comma(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a comma
      What we do depends on next character read

      If it is a hexadecimal digit then we have a valid number
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    /*
      If it is a dot or a comma then we have an invalid construct
    */
    if (test_char_sequence(c))
        return &state_hexadecimal_int_sequence;

    /*
      If it is a valid identifier character but not a valid hexadecimal
      digit then we have a hexadecimal integer with an invalid suffix
    */
    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_int_suffix;

    /*
      If it is anything else then it isn't part of the number
      This means that we have an integer terminated with a comma
      Report this finding to the caller
    */
    *token = TOK_HEX_INT_COM;

    return NULL;
}

static void *state_hexadecimal_int(token_t *token, int c)
{
    /*
      We have read some legal characters that can form a number
      We are sure that the last one was a hexadecimal digit or a comma
      What we do depends on next character read

      If it is a hexadecimal digit then we make a transition to this state
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    /*
      If it is a dot then we make this transition to next state
      It is up to it to decide whether it is a float or an invalid number
    */
    if (c == '.')
        return &state_hexadecimal_int_dot;

    /*
      If it is a comma then the number is valid so far. Function
      state_hexadecimal_int_comma will check if comma is followed by
      a valid character
    */
    if (c == ',')
        return &state_hexadecimal_int_comma;

    /*
      If it is a valid identifier character but invalid number
      character then we have a decimal integer with invalid suffix
    */
    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_int_suffix;

    /*
      If none of the transitions above was made then the character isn't
      part of the number. This means that this number is a valid integer
    */
    *token = TOK_HEX_INT;

    return NULL;
}

static void *state_hexadecimal_prefix(token_t *token, int c)
{
    /*
      So far we have read a '0x' suffix. If next character is
      a hexadecimal digit then we have a hexadecimal integer
    */
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    /*
      If this number is something else that can form a suffix
      then we have a decimal number 0 with a suffix
    */
    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    /*
      If none of the above transitions were made then we have 0 with suffix x
    */
    *token = TOK_DEC_INT_SUF;

    return NULL;
}

/*
  INITIAL ZERO STATE
  ==================
*/

static void *state_zero(token_t *token, int c)
{
    /*
      Zero is spacial in that it can form a prefix of binary, octal
      and hexadecimal numbers. So far we have read one zero

      If next character is a dicimal digit then we are dealing with
      a decimal number. Numbers like 01234 are decimal not octal
    */
    if (test_char_decimal(c))
        return &state_decimal_int;

    /*
      If next character is a dot then we have a decimal number
      that ends with a dot. We make transition to a state that will
      check if that number is a valid decimal float
    */
    if (c == '.')
        return &state_decimal_int_dot;

    /*
      If next character is a dot then we have a decimal number
      that ends with a dot. We make transition to a state that will
      check if that number is a valid decimal integer
    */
    if (c == ',')
        return &state_decimal_int_comma;

    /*
      Zero forms a binary prefix
    */
    if (c == 'b')
        return &state_binary_prefix;

    /*
      Zero forms an octal prefix
    */
    if (c == 'o')
        return &state_octal_prefix;

    /*
      Zero forma a hexadecimal prefix
    */
    if (c == 'x')
        return &state_hexadecimal_prefix;

    /*
      It's just zero, zero is a decimal integer
    */
    *token = TOK_DEC_INT;

    return NULL;
}

/*
  OTHER STATES
  ============
*/
static void *state_eol(token_t *token, int c)
{
    /*
      We don't care what next character is, we return EOL
    */
    *token = TOK_EOL;

    return NULL;
}

static void *state_end(token_t *token, int c)
{
    /*
      We don't care what next character is, we return END
    */
    *token = TOK_END;

    return NULL;
}

static void *state_unknown(token_t *token, int c)
{
    /*
      We have encountered unknown character, we don't care what is next
      We just return TOK_UNKNOWN
    */
    *token = TOK_UNKNOWN;

    return NULL;
}

/*
  INITIAL STATE
  =============
*/
static void *state_initial(token_t *token, int c)
{
    /*
      Lowercase identifiers
    */
    if (test_char_lowercase(c))
        return &state_lowercase;

    /*
      Uppercase identifiers
    */
    if (test_char_uppercase(c))
        return &state_uppercase;

    /*
      Number starting with zero, possibly binary, octal or hexadecimal number
    */
    if (c == '0')
        return &state_zero;

    /*
      Decimal number
    */
    if (test_char_decimal(c))
        return &state_decimal_int;

    /*
      End of line
    */
    if (c == '\n')
        return &state_eol;

    /*
      End of input
    */
    if (c == SOURCE_END)
        return &state_end;

    /*
      Unknown character
    */
    return &state_unknown;
}


lstatus_t rufum_scan(lunit_t **lunit_ptr, source_t *source)
{
    state_fn state;
    lstatus_t error;
    token_t token;
    lexme_info_t lexme_info;
    int c;

    /*
      Skip spaces, comments and escaped newlines
      Of skip fails we take care of errors
    */
    error = skip(source, &lexme_info);

    if (error != LEXER_OK)
        return handle_skip_error(lunit_ptr, &lexme_info, error);

    /*
      Initialize lexme, it stores information like line and column number,
      lexme value and its length
    */
    lexme_initialize(&lexme_info, source);

    /*
      This is our initial state, it's a function
    */
    state = &state_initial;

    /*
      Well, this is pretty simple but not obvious
      We read a character, append it to a buffer
      and then we make a transition to a different state
      What's so weird about it? That state is represented by
      a function that we can call to get next state (a function also)
      TODO Explain more
    */
    while (true)
    {
        /*
          Read one character
        */
        error = rufum_get_char(source, &c);

        if (error != LEXER_OK)
            return handle_error(&lexme_info, error);

        /*
          Make a transition to a different state based on character c
          NULL means there is no next state as accept operation took place
        */
        state = (state_fn) state(&token, c);

        if (state == NULL)
            break;

        /*
          If we made a transition then we append the character to the buffer
        */
        error = lexme_append(&lexme_info, c);

        if (error != LEXER_OK)
            return handle_error(&lexme_info, error);
    }

    /*
      Accept operation took place, this means that last character
      wasn't part of the lexme, unget it
    */
    error = rufum_unget_char(source, c);

    if (error != LEXER_OK)
        return handle_error(&lexme_info, error);

    /*
      Create a lunit
    */
    lunit_t *lunit;

    lunit = create_lunit(&lexme_info, token);

    if (lunit == NULL)
        return handle_error(&lexme_info, error);

    /*
      Make lunit accessible to the caller and indicate success
    */
    *lunit_ptr = lunit;

    return LEXER_OK;
}

void rufum_destroy_lunit(lunit_t *lunit)
{
    rufum_lstr_destroy(lunit->lexme);
    free(lunit);

    return;
}
