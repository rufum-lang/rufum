
#include "lexer.h"

#include "lstatus.h"
#include "lunit.h"

#include <common/lstring.h>
#include <common/memory.h>
#include <common/status.h>

#include <stdbool.h>
#include <stdlib.h>
/*
  These defines prevent us from having to pass the same
  argument twice over and over again
  We can pass a to a STATE_* macros instead both a and 'a'
  To convert a to 'a' we just write _TO_CHAR_##a
  which will expand to 'a'
*/
#define _TO_CHAR_a 'a'
#define _TO_CHAR_b 'b'
#define _TO_CHAR_c 'c'
#define _TO_CHAR_d 'd'
#define _TO_CHAR_e 'e'
#define _TO_CHAR_f 'f'
#define _TO_CHAR_g 'g'
#define _TO_CHAR_h 'h'
#define _TO_CHAR_i 'i'
#define _TO_CHAR_j 'j'
#define _TO_CHAR_k 'k'
#define _TO_CHAR_l 'l'
#define _TO_CHAR_m 'm'
#define _TO_CHAR_n 'n'
#define _TO_CHAR_o 'o'
#define _TO_CHAR_p 'p'
#define _TO_CHAR_q 'q'
#define _TO_CHAR_r 'r'
#define _TO_CHAR_s 's'
#define _TO_CHAR_t 't'
#define _TO_CHAR_u 'u'
#define _TO_CHAR_w 'w'
#define _TO_CHAR_x 'x'
#define _TO_CHAR_y 'y'
#define _TO_CHAR_z 'z'

/*
  This is the action that is the first thing that
  is done when transition occurs to a given state
  First we append character that lead us to this state
  to the lexme and then we read next character
  We could move the call to append_to_lexme to
  TRANSITION_* functions but that would mean that
  there are N calls to this function per state
  with N transitions. This way there is only one
  Also we must not forget to free lexme_info.text on failure
*/
#define PROLOGUE                             \
    error = append_to_lexme(&lexme_info, c); \
    if (error != LEXER_OK)                   \
    {                                        \
        free(lexme_info.text);               \
        return error;                        \
    }                                        \
    error = rufum_get_char(source, &c);      \
    if (error != LEXER_OK)                   \
    {                                        \
        free(lexme_info.text);               \
        return error;                        \
    }

/*
  A macro that implements a transition to a target
  if current character is char_a. S stands for signle
*/
#define TRANSITION_S(target, char_a) \
    if (c == char_a)                 \
        goto target;

/*
  A transition used by STATE_* macros. M stands for macro
  The transition is done if current character is char_suffix
  However this character isn't a char, that is it is missing quotes
  To convert it to an actual character we are using _TO_CHAR_* macros
  It is done this way so that we can write STATE_3(matched, a, b, c)
  instead of STATE_3(matched, a, 'a', b, 'b', c, 'c')
  Example: TRANSITION_M(retur, n)
    This transfers control to matched_return label
    if current character is 's'
*/
#define TRANSITION_M(matched, char_suffix) \
    if (c == _TO_CHAR_##char_suffix)       \
        goto matched_##matched##char_suffix;

/*
  This transition is a variation of TRANSITION_S
  It works similary except instead of comparing current character
  to a single character it compares it to a categery of characters
  Each category ${category} has a test_char_${category} function
  that checks if given character belongs to this it
  Capital C in macro name stands for category
*/
#define TRANSITION_C(target, function_suffix) \
    if (c == test_char_##function_suffix(c))  \
        goto target;

/*
  This macro represents accept action of finite state machine
  If rufum_unget_char fails we must free lexme_info.text
  to prevent memory leaks. When create_lunit fails
  it frees lexme_info.text so we don't have to do it here
*/
#define EPILOGUE(token)                       \
    error = rufum_unget_char(source, c);      \
    if (error != LEXER_OK)                    \
    {                                         \
        free(lexme_info.text);                \
        return error;                         \
    }                                         \
    lunit = create_lunit(&lexme_info, token); \
    if (lunit == NULL)                        \
        return LEXER_MEMORY_ERROR;

/*
  STATE_ID_* macros implement a finite state machine
  Search the web if you don't know what this means

  Number in a macro name defines how many transitions
  other than the default one this state can support
  From now on this number will be called N
  There are N+1 transitions in a STATE_N macro
  because there is a default transition to lowercase state
  This state (lowercasse) represents a lowercase identifier
  If macro's name ends with F it is a final state
  that accepts something other than TOK_LOWERCASE

  In all macros first parameter is name of a state
  Following N parameters are characters to be compared to the current char
  If the comparison returns true then a transition to associated state occurs
  Associated state for char c is matched_##current_state##c
  End macros have additional parameter: type of token to return
*/ 
#define STATE_ID_1(matched, char_a)         \
matched_##matched:                       \
    PROLOGUE                             \
    TRANSITION_M(matched, char_a)        \
    TRANSITION_C(lowercase, following)   \
    EPILOGUE(TOK_ID)

#define STATE_ID_2(matched, char_a, char_b) \
matched_##matched:                       \
    PROLOGUE                             \
    TRANSITION_M(matched, char_a)        \
    TRANSITION_M(matched, char_b)        \
    TRANSITION_C(lowercase, following)   \
    EPILOGUE(TOK_ID)

#define STATE_ID_F(matched, token)        \
matched_##matched:                     \
    PROLOGUE                           \
    TRANSITION_C(lowercase, following) \
    EPILOGUE(token)

#define TOKEN_INT_binary TOK_BIN_INT
#define TOKEN_INT_octal TOK_OCT_INT
#define TOKEN_INT_decimal TOK_DEC_INT
#define TOKEN_INT_hexadecimal TOK_HEX_INT

#define TOKEN_INT_DOT_binary TOK_BIN_INT_DOT
#define TOKEN_INT_DOT_octal TOK_OCT_INT_DOT
#define TOKEN_INT_DOT_decimal TOK_DEC_INT_DOT
#define TOKEN_INT_DOT_hexadecimal TOK_HEX_INT_DOT

#define TOKEN_INT_COM_binary TOK_BIN_INT_COM
#define TOKEN_INT_COM_octal TOK_OCT_INT_COM
#define TOKEN_INT_COM_decimal TOK_DEC_INT_COM
#define TOKEN_INT_COM_hexadecimal TOK_HEX_INT_COM

#define TOKEN_INT_SEQ_binary TOK_BIN_INT_SEQ
#define TOKEN_INT_SEQ_octal TOK_OCT_INT_SEQ
#define TOKEN_INT_SEQ_decimal TOK_DEC_INT_SEQ
#define TOKEN_INT_SEQ_hexadecimal TOK_HEX_INT_SEQ

#define TOKEN_INT_SUF_binary TOK_BIN_INT_SUF
#define TOKEN_INT_SUF_octal TOK_OCT_INT_SUF
#define TOKEN_INT_SUF_decimal TOK_DEC_INT_SUF
#define TOKEN_INT_SUF_hexadecimal TOK_HEX_INT_SUF

/*
  Following macros rely heavily on functions
  found in file categories.i.c included leter in this file
*/

/*
  Previous character was a digit but it is possible
  we have read one or more commas before that
  Following sequences of characters can lead here:
  '1', '2' ... '9', '369', '12,12', '0,3', '032' etc.
  Zero does not belong here, however this doesn't mean
  numbers starting with zero do not belong here
*/
#define STATE_INT(system)                              \
system##_int:                                          \
    PROLOGUE                                           \
    TRANSITION_C(system##_int, system)                 \
    TRANSITION_S(system##_int_dot, '.')                \
    TRANSITION_S(system##_int_comma, ',')              \
    TRANSITION_C(system##_int_suffix, system##_suffix) \
    EPILOGUE(TOKEN_INT_##system)

/*
  We have encountered a dot after sequence of digits and optionally
  one or more commas, previous character was a digit though
  If we encounter a digit then we have a float
  Example: We just got '0' after "1." which gives us "1.0"
  However if we get another dot or comma then we have an invalid sequence
*/
#define STATE_INT_DOT(system)                     \
system##_int_dot:                                 \
    PROLOGUE                                      \
    TRANSITION_C(system##_float, system)          \
    TRANSITION_C(system##_int_sequence, sequence) \
    EPILOGUE(TOKEN_INT_DOT_##system)

#define STATE_INT_COMMA(system)                   \
system##_int_comma:                               \
    PROLOGUE                                      \
    TRANSITION_C(system##_int, system)            \
    TRANSITION_C(system##_int_sequence, sequence) \
    EPILOGUE(TOKEN_INT_COM_##system)

#define STATE_INT_SEQUENCE(system)                 \
system##_int_sequence:                             \
    PROLOGUE                                       \
    TRANSITION_C(system##_int_sequence, following) \
    EPILOGUE(TOKEN_INT_SEQ_##system)

#define STATE_INT_SUFFIX(system)                 \
system##_int_suffix:                             \
    PROLOGUE                                     \
    TRANSITION_C(system##_int_suffix, following) \
    EPILOGUE(TOKEN_INT_SUF_##system)

#define INTEGER_STATES(system) \
    STATE_INT(system)          \
    STATE_INT_DOT(system)      \
    STATE_INT_COMMA(system)    \
    STATE_INT_SEQUENCE(system) \
    STATE_INT_SUFFIX(system)

#define TOKEN_FLT_binary TOK_BIN_FLT
#define TOKEN_FLT_octal TOK_OCT_FLT
#define TOKEN_FLT_decimal TOK_DEC_FLT
#define TOKEN_FLT_hexadecimal TOK_HEX_FLT

#define TOKEN_FLT_DOT_binary TOK_BIN_FLT_DOT
#define TOKEN_FLT_DOT_octal TOK_OCT_FLT_DOT
#define TOKEN_FLT_DOT_decimal TOK_DEC_FLT_DOT
#define TOKEN_FLT_DOT_hexadecimal TOK_HEX_FLT_DOT

#define TOKEN_FLT_COM_binary TOK_BIN_FLT_COM
#define TOKEN_FLT_COM_octal TOK_OCT_FLT_COM
#define TOKEN_FLT_COM_decimal TOK_DEC_FLT_COM
#define TOKEN_FLT_COM_hexadecimal TOK_HEX_FLT_COM

#define TOKEN_FLT_SEQ_binary TOK_BIN_FLT_SEQ
#define TOKEN_FLT_SEQ_octal TOK_OCT_FLT_SEQ
#define TOKEN_FLT_SEQ_decimal TOK_DEC_FLT_SEQ
#define TOKEN_FLT_SEQ_hexadecimal TOK_HEX_FLT_SEQ

#define TOKEN_FLT_SUF_binary TOK_BIN_FLT_SUF
#define TOKEN_FLT_SUF_octal TOK_OCT_FLT_SUF
#define TOKEN_FLT_SUF_decimal TOK_DEC_FLT_SUF
#define TOKEN_FLT_SUF_hexadecimal TOK_HEX_FLT_SUF

#define STATE_FLOAT(system)                              \
system##_float:                                          \
    PROLOGUE                                             \
    TRANSITION_C(system##_float, system)                 \
    TRANSITION_S(system##_float_dot, '.')                \
    TRANSITION_S(system##_float_comma, ',')              \
    TRANSITION_C(system##_float_suffix, system##_suffix) \
    EPILOGUE(TOKEN_FLT_##system)

#define STATE_FLOAT_DOT(system)                 \
system##_float_dot:                             \
    PROLOGUE                                    \
    TRANSITION_C(system##_float_dot, following) \
    EPILOGUE(TOKEN_FLT_DOT_##system)

#define STATE_FLOAT_COMMA(system)                   \
system##_float_comma:                               \
    PROLOGUE                                        \
    TRANSITION_C(system##_float, system)            \
    TRANSITION_C(system##_float_sequence, sequence) \
    EPILOGUE(TOKEN_FLT_COM_##system)

#define STATE_FLOAT_SEQUENCE(system)                 \
system##_float_sequence:                             \
    PROLOGUE                                         \
    TRANSITION_C(system##_float_sequence, following) \
    EPILOGUE(TOKEN_FLT_SEQ_##system)

#define STATE_FLOAT_SUFFIX(system)                 \
system##_float_suffix:                             \
    PROLOGUE                                       \
    TRANSITION_C(system##_float_suffix, following) \
    EPILOGUE(TOKEN_FLT_SUF_##system)

#define FLOAT_STATES(system)     \
    STATE_FLOAT(system)          \
    STATE_FLOAT_DOT(system)      \
    STATE_FLOAT_COMMA(system)    \
    STATE_FLOAT_SEQUENCE(system) \
    STATE_FLOAT_SUFFIX(system)

// TODO move it to config file
#define LEXME_ALLOCATION_STEP 32

struct lexme_info
{
    char *text;
    size_t line;
    size_t column;
    size_t index;
    size_t space;
};

typedef struct lexme_info lexme_info_t;

#include "skip.i.c"

#include "categories.i.c"

static lstatus_t append_to_lexme(lexme_info_t *lexme_info, char c)
{
    /*
      Check if there is space for a new character
      If not we are going to reallocate the buffer
    */
    if (lexme_info->index == lexme_info->space)
    {
        size_t new_size;
        char *new_text;

        /*
          Calling realloc every time we append a character
          would be very inefficient, instead we do it
          every LEXME_ALLOCATION_STEP character added
          lexme_info->text is initialized to NULL in rufum_get_lunit
          This means when appending character for the first time
          realloc will behave like malloc
        */
        new_size = lexme_info->space + LEXME_ALLOCATION_STEP;
        new_text = realloc(lexme_info->text, new_size);

        if (new_text == NULL)
            return LEXER_MEMORY_ERROR;

        /*
          If we assigned result of realloc directly to lexme_info->text
          and realloc returned NULL we would have a memory leak
        */
        lexme_info->space = new_size;
        lexme_info->text = new_text;
    }

    /*
      Append the character incrementing the index member
    */
    lexme_info->text[lexme_info->index] = c;
    lexme_info->index += 1;

    return LEXER_OK;
}

static lunit_t *create_lunit(lexme_info_t *lexme_info, enum token token)
{
    lunit_t *lunit;

    /*
      Our task is to create a lunit (lexer unit)
      and populate it with information
      First we need to allocate it
    */
    NEW(lunit)

    if (lunit == NULL)
        return NULL;

    char *new_text;

    new_text = realloc(lexme_info->text, lexme_info->index);

    if (new_text == NULL)
    {
        free(lunit);

        return NULL;
    }

    /*
      Allocate a structure to hold information about
      lexme's value and length
      If we fail we also need to free lunit and new_text
      in order to prevent memory leaks
    */
    lunit->lexme = lstring_from_bytes(new_text, lexme_info->index);

    if (lunit->lexme == NULL)
    {
        free(new_text);
        free(lunit);

        return NULL;
    }


    lunit->lexme->text = realloc(lexme_info->text, lexme_info->index);

    if (lunit->lexme->text == NULL)
    {
        lstring_destroy(lunit->lexme);
        free(lunit);

        return NULL;
    }

    // TODO

    return lunit;
}

lstatus_t rufum_get_lunit(lunit_t **lunit_ptr, source_t *source)
{
    lstatus_t error;
    lexme_info_t lexme_info;
    lunit_t *lunit;
    int c;

    /*
      We are going to realloc this so we have to initialize it to NULL
    */
    lexme_info.text = NULL;
    
    /*
      We haven't allocated any space for the lexme (space)
      and we haven't appended any characters yet (index)
    */
    lexme_info.index = 0;
    lexme_info.space = 0;

    /*
      Skip spaces, comments and escaped newlines
      If after the call error isn't equal to LEXER_OK
      then there were an error
    */
    error = skip(source, &lexme_info);

    /*
      This happens when we encounter a comment not terminated by newline
      We return a special token
    */
    if (error == LEXER_BAD_COMMENT)
    {
        lunit = create_lunit(&lexme_info, TOK_BAD_COM);

        /*
          Memory allocation failure is more serious than
          unterminated comment because it is quite likely that
          we will be unable to allocate anything
        */
        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;

        return error;
    }

    /*
      This happens when we encounter unterminated multiline comment
      We return a special token
    */
    if (error == LEXER_BAD_MULTILINE_COMMENT)
    {
        lunit = create_lunit(&lexme_info, TOK_BAD_ML_COM);

        /*
          Memory allocation failure is more serious than
          unterminated comment because it is quite likely that
          we will be unable to allocate anything
        */
        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;

        return error;
    }

    /*
      If skip() returned any other error we just return it
      No need to free anything other than source (this is parser's task)
    */
    if (error != LEXER_OK)
        return error;

    /*
      Now that we have skipped all the things that
      shouldn't be returned as tokens
      we can finally start doing our job
      This is the first state of finite state machine
      Save position of current character and then
      read one char and based on it move to an appropriate state
    */
    lexme_info.line = rufum_get_line(source);
    lexme_info.column = rufum_get_column(source);

    error = rufum_get_char(source, &c);

    if (error != LEXER_OK)
        return error;
        

    /*
      Neither this if statement nor this switch represent a state
      Their task is to move to an appropriate state, nothing more
    */
    if (test_char_lowercase(c))
    {
        switch (c)
        {
            default:
                goto lowercase;
        }
    }

    if (c == '0')
        goto initial_zero;

    if (test_char_decimal(c))
        goto decimal_int;

    if (c == '\n')
    {
        lunit = create_lunit(&lexme_info, TOK_EOL);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;
        return LEXER_OK;
    }

    if (c == SOURCE_END)
    {
        lunit = create_lunit(&lexme_info, TOK_END);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;
        return LEXER_OK;
    }

    lunit = create_lunit(&lexme_info, TOK_UNKNOWN);

    if (lunit == NULL)
        return LEXER_MEMORY_ERROR;

    *lunit_ptr = lunit;
    return LEXER_OK;

initial_zero:
    /*
      We have got a zero and this is first character read
      We need to destinguish initial_zero from decimal_int
      because zero can form 0b, 0o and 0x prefixes
      Otherwise it works just like decimal_int
    */
    PROLOGUE
    TRANSITION_C(decimal_int, decimal)
    TRANSITION_S(decimal_int_dot, '.')
    TRANSITION_S(decimal_int_comma, ',')
    TRANSITION_S(binary_prefix, 'b')
    TRANSITION_S(octal_prefix, 'o')
    TRANSITION_S(hexadecimal_prefix, 'x')
    EPILOGUE(TOK_DEC_INT)

binary_prefix:
    PROLOGUE
    TRANSITION_C(binary_int, binary)
    EPILOGUE(TOK_DEC_INT_SUF)

octal_prefix:
    PROLOGUE
    TRANSITION_C(octal_int, octal)
    EPILOGUE(TOK_DEC_INT_SUF)

hexadecimal_prefix:
    PROLOGUE
    TRANSITION_C(hexadecimal_int, hexadecimal)
    EPILOGUE(TOK_DEC_INT_SUF)

    /*
      These macros expand to multiple states
      responsible for scanning numbers
    */
    INTEGER_STATES(binary)
    INTEGER_STATES(octal)
    INTEGER_STATES(decimal)
    INTEGER_STATES(hexadecimal)
    FLOAT_STATES(binary)
    FLOAT_STATES(octal)
    FLOAT_STATES(decimal)
    FLOAT_STATES(hexadecimal)

lowercase:
    PROLOGUE
    TRANSITION_C(lowercase, lowercase)
    EPILOGUE(TOK_LOWERCASE)
}
