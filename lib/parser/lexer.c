
#include "lexer.h"

#include "lstatus.h"
#include "lunit.h"

#include <common/lstring.h>
#include <common/memory.h>
#include <common/status.h>

#include <stdbool.h>
#include <stdlib.h>

/*
  BASIC MACROS
  ============

  These defines prevent us from having to pass the same
  argument twice over and over again
  We can pass a to STATE_KW_* macros instead both a and 'a'
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
  KEYWORD STATE MACROS
  ====================

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
#define STATE_KW_1(matched, char_a)         \
matched_##matched:                       \
    PROLOGUE                             \
    TRANSITION_M(matched, char_a)        \
    TRANSITION_C(lowercase, following)   \
    EPILOGUE(TOK_ID)

#define STATE_KW_2(matched, char_a, char_b) \
matched_##matched:                       \
    PROLOGUE                             \
    TRANSITION_M(matched, char_a)        \
    TRANSITION_M(matched, char_b)        \
    TRANSITION_C(lowercase, following)   \
    EPILOGUE(TOK_ID)

#define STATE_KW_F(matched, token)        \
matched_##matched:                     \
    PROLOGUE                           \
    TRANSITION_C(lowercase, following) \
    EPILOGUE(token)

/*
  INTEGER STATE MACROS
  ===================

  These macros are used select right token to return
  They are used by STATE_INT_* macros
  Example:
    Macro STATE_INT takes system as a parameter and uses TOKEN_INT_##system
    to find right token to return depending on numeral system used
*/
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

  In STATE_INT_* and STATE_FLOAT_* macros what we mean by digit depends
  on the numeral system, when we say we check if it is a digit
  we mean 'it might be 0-1 or 0-7 or 0-9 or 0-9 a-f A-F'

  There are two kinds of suffix characters: initial suffix characters
  and successive suffix characters. The former are characters that can form
  an identifier but are invalid in context of a number. To get a set
  of suffix characters for a given numeral system we need to do a set
  difference of characters that make up identifiers and characters that make
  up a number in a given numeral system. Example:
    identifiers: a-z A-Z 0-9 ? _ 
    octal: 0-7 , . 
    difference: a-z A-Z 8-9 ? _
  The differnece set is a set of initial suffix characters for octal numbers.
  
  Successive suffix characters follow initial suffix character
  To get this set we need to perform a union of identifier characters and
  characters making up a number in a given numeral system. Example:
    identidiers: a-z A-Z 0-9 ? _
    hexadecimal: 0-9 a-f A-F , .
    union: a-z A-Z 0-9 ? _ , .
  Note tham union will be the same no matter which numeral system is used
*/

/*
  Previous character was a digit but it is possible
  we have read one or more commas before that
  Zero does not belong here, however this doesn't mean
  numbers starting with zero do not belong here
  Current state: '1', '3,69', '0b0', '0x3,4', '0o70', '01'
  First transition occurs when we encounter another digit
  If we do encounter it we move to this state again (STATE_INT)
  Examples: '11', '3,690', '0b01', '0x3,14', '0o700', '018'
  Second transition occurs when we encounter a dot
  It leads to state represented by STATE_INT_DOT macro
  Examples: '1.', '3,69.', '0b0.', '0x3,1.', '0o70.', '01.'
  Third transition occurs when we encounter a comma
  It leads to state represented by STATE_INT_COMMA macro
  Examples: '1,', '3,69,', '0b0,', '0x3,1,', '0o70,', '01,'
  Fourth transition occurs when we encounter a character
  that begins a suffix, what is considered such a character
  depends on numeral system used.
  It leads to a state represented by STATE_INT_SUFFIX macro
  Examples: '1_', '3,69a', '0b02', '0x3,1g', '0o708', '01?'
  If none of the transitions occurs then we have found a valid integer
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
  Current state: '1.', '3,000.', '0b0.', '0x3.', '0o333.', '004.'
  First transition occurs when we encounter a digit
  It leads to a state represented by STATE_FLOAT macro
  Examples: '1.3', '3,000.0', '0b0.1', '0x3.1', '0o333.3', '004.9'
  Second transition occurs whenwe encounter a dot or a comma
  Any number that has a comma followed by a dot or comma is invalid
  This transition leads to a state represented by STATE_INT_SEQUENCE macro
  Examples: '1..', 13,000.,', '0b0.,', '0x3..', '0o333..', '004.,'
  Third transition occurs when we encounter a initial suffix character
  This transition leads to a state represented by STATE_FLOAT_SUFFIX macro
  Example: '1.a', '3,000._', '0b0.2', '0x3.g', '0o333.z', '004.?'
  It is also possible that none of the transitions will be made
  In this case we report a number terminated by a dot
*/
#define STATE_INT_DOT(system)                            \
system##_int_dot:                                        \
    PROLOGUE                                             \
    TRANSITION_C(system##_float, system)                 \
    TRANSITION_C(system##_int_sequence, sequence)        \
    TRANSITION_C(system##_float_suffix, system##_suffix) \
    EPILOGUE(TOKEN_INT_DOT_##system)

/*
  We have encountered a comma after a sequence of digits and optionally
  one or more commas, previous character was a digit though
  Current state: '23,', '2,300,', '0b1110,', '0xfee1,', '0o770,', '0,0,3,'
  First transition occurs when we encounter a digit
  It leads to a state represented by STATE_INT macro
  Examples: '23,4', '2,300,4', '0b1110,0', '0xfee1,0', '0o770,7', '0,0,3,2'
  Second transition occurs when we encounter a dot or comma
  A comma followed by another comma or a dot forms an invalid sequence
  This transition leads to a state represented by STATE_INT_SEQUENCE macro
  Examples: '23,,', '2,300,.', '0b1110,,', '0xfee1,.', '0o770,,', '0,0,3,.'
  Third transition occurs when we encounter a initial suffix character
  This transition leads to a state represented by STATE_INT_SUFFIX
  Examples: '23,b', '2,300,F', '0b1110,4', '0xfee1,k', '0o777,9', '0,0,3,R'
  It is also possible that none of the transition will be made
  In this case we report a number terminated by a comma
*/
#define STATE_INT_COMMA(system)                        \
system##_int_comma:                                    \
    PROLOGUE                                           \
    TRANSITION_C(system##_int, system)                 \
    TRANSITION_C(system##_int_sequence, sequence)      \
    TRANSITION_C(system##_int_suffix, system##_suffix) \
    EPILOGUE(TOKEN_INT_COM_##system)

/*
  We have encountered a number but it contained a dot or
  a comma followed by another dot or a comma
  Current state: '2,,', '200,300,.', '3,14..', '1,618.,'
  There is only one transition: we read remaining characters until
  we encounter a character that cannot be part of identifier nor number
  When we finish we indicate we found an invalid number
*/
#define STATE_INT_SEQUENCE(system)              \
system##_int_sequence:                          \
    PROLOGUE                                    \
    TRANSITION_C(system##_int_sequence, suffix) \
    EPILOGUE(TOKEN_INT_SEQ_##system)

/*
  We have encountered a character that can form an identifier
  but connot form a number in a given numeral system
  There is only one transition: we read characters until
  we encounter something that cannot be part of identifier nor number
  When we finish we indicate we found an invalid number
*/
#define STATE_INT_SUFFIX(system)              \
system##_int_suffix:                          \
    PROLOGUE                                  \
    TRANSITION_C(system##_int_suffix, suffix) \
    EPILOGUE(TOKEN_INT_SUF_##system)

/*
  This macro creates all the states needed to scan a number
  in a given numeral system, it depends on FLOAT_STATES though
  as there are transition to floating point number states
*/
#define INTEGER_STATES(system) \
    STATE_INT(system)          \
    STATE_INT_DOT(system)      \
    STATE_INT_COMMA(system)    \
    STATE_INT_SEQUENCE(system) \
    STATE_INT_SUFFIX(system)

/*
  FLOATING POINT NUMBER STATE MACROS
  ==================================

  These macros are used select right token to return
  They are used by STATE_FLT_* macros
  Example:
    Macro STATE_FLOAT takes system as a parameter and uses TOKEN_FLT_##system
    to find right token to return depending on numeral system used
*/
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

/*
  Identical to STATE_INT except we have encountered one dot in addition to
  commas and numbers, last character was either a dot or a number
  First transition leads to this macro
  Second transition leads to a state represented by STATE_FLOAT_DOT macro
  Third transition leads to a state represented by STATE_FLOAT_COMMA macro
  Fourth state leads to a state represented by STATE_FLOAT_SUFFIX macro
  If we can't make a transition then we have found a legal float
*/
#define STATE_FLOAT(system)                              \
system##_float:                                          \
    PROLOGUE                                             \
    TRANSITION_C(system##_float, system)                 \
    TRANSITION_S(system##_float_dot, '.')                \
    TRANSITION_S(system##_float_comma, ',')              \
    TRANSITION_C(system##_float_suffix, system##_suffix) \
    EPILOGUE(TOKEN_FLT_##system)

/*
  We have encountered another dot but there can be only one
  There is only one transition leading back to this macro
  We sort of loop reading characters until we encounter
  something that can't form a suffix
*/
#define STATE_FLOAT_DOT(system)              \
system##_float_dot:                          \
    PROLOGUE                                 \
    TRANSITION_C(system##_float_dot, suffix) \
    EPILOGUE(TOKEN_FLT_DOT_##system)

/*
  This works exactly the same way as STATE_INT_COMMA
*/
#define STATE_FLOAT_COMMA(system)                   \
system##_float_comma:                               \
    PROLOGUE                                        \
    TRANSITION_C(system##_float, system)            \
    TRANSITION_C(system##_float_sequence, sequence) \
    TRANSITION_C(system##_float_suffix, suffix)     \
    EPILOGUE(TOKEN_FLT_COM_##system)

/*
  This works exactly the same way as STATE_INT_SEQUENCE
*/
#define STATE_FLOAT_SEQUENCE(system)              \
system##_float_sequence:                          \
    PROLOGUE                                      \
    TRANSITION_C(system##_float_sequence, suffix) \
    EPILOGUE(TOKEN_FLT_SEQ_##system)

/*
  This works exactly the same way as STATE_INT_SUFFIX
*/
#define STATE_FLOAT_SUFFIX(system)              \
system##_float_suffix:                          \
    PROLOGUE                                    \
    TRANSITION_C(system##_float_suffix, suffix) \
    EPILOGUE(TOKEN_FLT_SUF_##system)

/*
  This macro creates all the states needed to scan
  a floating point number in a given numeral system
*/
#define FLOAT_STATES(system)     \
    STATE_FLOAT(system)          \
    STATE_FLOAT_DOT(system)      \
    STATE_FLOAT_COMMA(system)    \
    STATE_FLOAT_SEQUENCE(system) \
    STATE_FLOAT_SUFFIX(system)

/*
  DATA SECTION
  ============
*/

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

/*
  CODE INCLUDE SECTION
  ====================

  These two files contain static C functions
  They are in separate files because this file is already too big
  These functions are similar in what they do:
    skip.i.c: here we skip whitespace and comments
    categories.i.c: here we compare characters to certain character categories
*/
#include "skip.i.c"
#include "categories.i.c"

/*
  CODE SECTION
  ============

  Here begin the actual function definitions
  Following functions are defined:
    append_to_lexme: This function appends a character to a string
      representing currently processed token (a lexme)
    create_lunit: This function processes lexme_info in order to create a 
      lunit or lexer unit, that is the thing that lexer outputs
      It contains all the information about the token that parser needs
    rufum_get_lunit: Main lexer function, it implements a finite state machine
      and outputs a lunit
*/

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

    /*
      lexme_info->text was allocated in steps
      Each step occured every LEXME_ALLOCATION_STEP characters
      Currently we have lexme_info->space space allocated
      But need only lexme_info->index bytes, where index <= space
      This means that we should trim space used by the lexme
      Note: Realloc can fail even when trimming memory region
    */
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

    lunit->token = token;
    lunit->line = lexme_info->line;
    lunit->column = lexme_info->column;

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
      Handle keywords and lowercase identifiers
    */
    TRANSITION_C(lowercase, lowercase)

    /*
      Handle uppercase identifiers
    */
    TRANSITION_C(uppercase, uppercase)

    /*
      Handle numbers, zero is a standalone state bacause it
      can form prefix of binary, octal and hexadecimal numbers
    */
    TRANSITION_S(initial_zero, '0')
    TRANSITION_C(decimal_int, decimal)

    /*
      Handle end of line
    */
    if (c == '\n')
    {
        lunit = create_lunit(&lexme_info, TOK_EOL);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;
        return LEXER_OK;
    }

    /*
      Handle end of input
    */
    if (c == SOURCE_END)
    {
        lunit = create_lunit(&lexme_info, TOK_END);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;
        return LEXER_OK;
    }

    /*
      Handle unknown entity
    */
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

uppercase:
    PROLOGUE
    TRANSITION_C(uppercase, uppercase)
    EPILOGUE(TOK_UPPERCASE)
}
