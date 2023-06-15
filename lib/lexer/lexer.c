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
    handle_error and create_lunit
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
    if (test_char_following(c))
        return &state_lowercase;

    *token = TOK_LOWERCASE;

    return NULL;
}

static void *state_uppercase(token_t *token , int c)
{
    if (test_char_following(c))
        return &state_uppercase;

    *token = TOK_UPPERCASE;

    return NULL;
}

/*
  DECIMAL FLOATING POINT STATES
  ======================

  This declaration is needed because state_decimal_float
  calls state_decimal_float_comma and vice versa
*/
static void *state_decimal_float(token_t *token, int c);


static void *state_decimal_float_suffix(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_decimal_float_suffix;

    *token = TOK_DEC_FLT_SUF;

    return NULL;
}

static void *state_decimal_float_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_decimal_float_sequence;

    *token = TOK_DEC_FLT_SEQ;

    return NULL;
}

static void *state_decimal_float_dot(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_float;

    if (test_char_sequence(c))
        return &state_decimal_float_sequence;

    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    *token = TOK_DEC_FLT_DOT;

    return NULL;
}

static void *state_decimal_float_comma(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_float;

    if (test_char_sequence(c))
        return &state_decimal_float_sequence;

    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    *token = TOK_DEC_FLT_COM;

    return NULL;
}

static void *state_decimal_float(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_float;

    if (c == '.')
        return &state_decimal_float_dot;

    if (c == ',')
        return &state_decimal_float_comma;

    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

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
    if (test_char_suffix(c))
        return &state_binary_float_suffix;

    *token = TOK_BIN_FLT_SUF;

    return NULL;
}

static void *state_binary_float_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_binary_float_sequence;

    *token = TOK_BIN_FLT_SEQ;

    return NULL;
}

static void *state_binary_float_dot(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_float;

    if (test_char_sequence(c))
        return &state_binary_float_sequence;

    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    *token = TOK_BIN_FLT_DOT;

    return NULL;
}

static void *state_binary_float_comma(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_float;

    if (test_char_sequence(c))
        return &state_binary_float_sequence;

    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    *token = TOK_BIN_FLT_COM;

    return NULL;
}

static void *state_binary_float(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_float;

    if (c == '.')
        return &state_binary_float_dot;

    if (c == ',')
        return &state_binary_float_comma;

    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

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
    if (test_char_suffix(c))
        return &state_octal_float_suffix;

    *token = TOK_OCT_FLT_SUF;

    return NULL;
}

static void *state_octal_float_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_octal_float_sequence;

    *token = TOK_OCT_FLT_SEQ;

    return NULL;
}

static void *state_octal_float_dot(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_float;

    if (test_char_sequence(c))
        return &state_octal_float_sequence;

    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    *token = TOK_OCT_FLT_DOT;

    return NULL;
}

static void *state_octal_float_comma(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_float;

    if (test_char_sequence(c))
        return &state_octal_float_sequence;

    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    *token = TOK_OCT_FLT_COM;

    return NULL;
}

static void *state_octal_float(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_float;

    if (c == '.')
        return &state_octal_float_dot;

    if (c == ',')
        return &state_octal_float_comma;

    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

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
    if (test_char_suffix(c))
        return &state_hexadecimal_float_suffix;

    *token = TOK_HEX_FLT_SUF;

    return NULL;
}

static void *state_hexadecimal_float_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_hexadecimal_float_sequence;

    *token = TOK_HEX_FLT_SEQ;

    return NULL;
}

static void *state_hexadecimal_float_dot(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    if (test_char_sequence(c))
        return &state_hexadecimal_float_sequence;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    *token = TOK_HEX_FLT_DOT;

    return NULL;
}

static void *state_hexadecimal_float_comma(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    if (test_char_sequence(c))
        return &state_hexadecimal_float_sequence;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    *token = TOK_HEX_FLT_COM;

    return NULL;
}

static void *state_hexadecimal_float(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    if (c == '.')
        return &state_hexadecimal_float_dot;

    if (c == ',')
        return &state_hexadecimal_float_comma;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

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
    if (test_char_suffix(c))
        return &state_decimal_int_suffix;

    *token = TOK_DEC_INT_SUF;

    return NULL;
}

static void *state_decimal_int_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_decimal_int_sequence;

    *token = TOK_DEC_INT_SEQ;

    return NULL;
}

static void *state_decimal_int_dot(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_float;

    if (test_char_sequence(c))
        return &state_decimal_int_sequence;

    if (test_char_decimal_suffix(c))
        return &state_decimal_float_suffix;

    *token = TOK_DEC_INT_DOT;

    return NULL;
}

static void *state_decimal_int_comma(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_int;

    if (test_char_sequence(c))
        return &state_decimal_int_sequence;

    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

    *token = TOK_DEC_INT_COM;

    return NULL;
}

static void *state_decimal_int(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_int;

    if (c == '.')
        return &state_decimal_int_dot;

    if (c == ',')
        return &state_decimal_int_comma;

    if (test_char_decimal_suffix(c))
        return &state_decimal_int_suffix;

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
    if (test_char_suffix(c))
        return &state_binary_int_suffix;

    *token = TOK_BIN_INT_SUF;

    return NULL;
}

static void *state_binary_int_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_binary_int_sequence;

    *token = TOK_BIN_INT_SEQ;

    return NULL;
}

static void *state_binary_int_dot(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_float;

    if (test_char_sequence(c))
        return &state_binary_int_sequence;

    if (test_char_binary_suffix(c))
        return &state_binary_float_suffix;

    *token = TOK_BIN_INT_DOT;

    return NULL;
}

static void *state_binary_int_comma(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_int;

    if (test_char_sequence(c))
        return &state_binary_int_sequence;

    if (test_char_binary_suffix(c))
        return &state_binary_int_suffix;

    *token = TOK_BIN_INT_COM;

    return NULL;
}

static void *state_binary_int(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_int;

    if (c == '.')
        return &state_binary_int_dot;

    if (c == ',')
        return &state_binary_int_comma;

    if (test_char_binary_suffix(c))
        return &state_binary_int_suffix;

    *token = TOK_BIN_INT;

    return NULL;
}

static void *state_binary_prefix(token_t *token, int c)
{
    if (test_char_binary(c))
        return &state_binary_int;

    return &state_decimal_int_suffix;
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
    if (test_char_suffix(c))
        return &state_octal_int_suffix;

    *token = TOK_OCT_INT_SUF;

    return NULL;
}

static void *state_octal_int_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_octal_int_sequence;

    *token = TOK_OCT_INT_SEQ;

    return NULL;
}

static void *state_octal_int_dot(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_float;

    if (test_char_sequence(c))
        return &state_octal_int_sequence;

    if (test_char_octal_suffix(c))
        return &state_octal_float_suffix;

    *token = TOK_OCT_INT_DOT;

    return NULL;
}

static void *state_octal_int_comma(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_int;

    if (test_char_sequence(c))
        return &state_octal_int_sequence;

    if (test_char_octal_suffix(c))
        return &state_octal_int_suffix;

    *token = TOK_OCT_INT_COM;

    return NULL;
}

static void *state_octal_int(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_int;

    if (c == '.')
        return &state_octal_int_dot;

    if (c == ',')
        return &state_octal_int_comma;

    if (test_char_octal_suffix(c))
        return &state_octal_int_suffix;

    *token = TOK_OCT_INT;

    return NULL;
}

static void *state_octal_prefix(token_t *token, int c)
{
    if (test_char_octal(c))
        return &state_octal_int;

    return &state_decimal_int_suffix;
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
    if (test_char_suffix(c))
        return &state_hexadecimal_int_suffix;

    *token = TOK_HEX_INT_SUF;

    return NULL;
}

static void *state_hexadecimal_int_sequence(token_t *token, int c)
{
    if (test_char_suffix(c))
        return &state_hexadecimal_int_sequence;

    *token = TOK_HEX_INT_SEQ;

    return NULL;
}

static void *state_hexadecimal_int_dot(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_float;

    if (test_char_sequence(c))
        return &state_hexadecimal_int_sequence;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_float_suffix;

    *token = TOK_HEX_INT_DOT;

    return NULL;
}

static void *state_hexadecimal_int_comma(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    if (test_char_sequence(c))
        return &state_hexadecimal_int_sequence;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_int_suffix;

    *token = TOK_HEX_INT_COM;

    return NULL;
}

static void *state_hexadecimal_int(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    if (c == '.')
        return &state_hexadecimal_int_dot;

    if (c == ',')
        return &state_hexadecimal_int_comma;

    if (test_char_hexadecimal_suffix(c))
        return &state_hexadecimal_int_suffix;

    *token = TOK_HEX_INT;

    return NULL;
}

static void *state_hexadecimal_prefix(token_t *token, int c)
{
    if (test_char_hexadecimal(c))
        return &state_hexadecimal_int;

    return &state_decimal_int_suffix;
}

/*
  INITIAL ZERO STATE
  ==================
*/

static void *state_zero(token_t *token, int c)
{
    if (test_char_decimal(c))
        return &state_decimal_int;

    if (c == '.')
        return &state_decimal_int_dot;

    if (c == ',')
        return &state_decimal_int_comma;

    if (c == 'b')
        return &state_binary_prefix;

    if (c == 'o')
        return &state_octal_prefix;

    if (c == 'x')
        return &state_hexadecimal_prefix;

    *token = TOK_DEC_INT;

    return NULL;
}

/*
  OTHER STATES
  ============
*/
static void *state_eol(token_t *token, int c)
{
    *token = TOK_EOL;

    return NULL;
}

static void *state_end(token_t *token, int c)
{
    *token = TOK_END;

    return NULL;
}

static void *state_unknown(token_t *token, int c)
{
    *token = TOK_UNKNOWN;

    return NULL;
}

/*
  INITIAL STATE
  =============
*/
static void *state_initial(token_t *token, int c)
{
    if (test_char_lowercase(c))
        return &state_lowercase;

    if (test_char_uppercase(c))
        return &state_uppercase;

    if (c == '0')
        return &state_zero;

    if (test_char_decimal(c))
        return &state_decimal_int;

    if (c == '\n')
        return &state_eol;

    if (c == SOURCE_END)
        return &state_end;

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
