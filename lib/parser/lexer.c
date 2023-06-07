
#include "lexer.h"

#include "lunit.h"

#include <common/lstring.h>
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

#define PROLOGUE                     \
    append_to_lexme(&lexme_info, c); \
    c = rufum_get_char(source);

#define TRANSITION_S(target, char_a) \
    if (c == char_a)               \
        goto target;

#define TRANSITION_M(matched, char_suffix) \
    if (c == _TO_CHAR_##char_suffix)       \
        goto matched_##matched##target_suffix;

#define TRANSITION_F(target, function_suffix) \
    if (c == test_char_##function_suffix(c))  \
        goto target;

#define EPILOGUE(token)            \
    rufum_unget_char(source, c); \
    return create_lunit(&lexme_info, token);

/*
  STATE_* macros implement a finite state machine
  Search the web if you don't know what this means
  Different states are represented by labels and
  transition between them is done with a goto instruction

  Number in a macro name defines how many transitions
  other than the default one state can support
  From now on this number will be called N
  There are N+1 transitions in a STATE_N macro
  because there is a default transition to ident state
  This state (ident) represents an identifier
  If macro's name ends with F it is a final state
  that accepts something other than TOK_IDENTIFIER

  In all macros first parameter is name of a state
  Following N parameters are characters to be compared to the current char
  If the comparison returns true then a transition to associated state occurs
  Associated state for char c is matched_##current_state##c
  End macros have additional parameter: type of token to return

  Operations that STATE_* macros perform
  epilogue:
    it appends input of previous state to the lexme
    Example: lexme_append(&lexme_info, c);
  prologue:
    it retrives one character from the source
    Example: rufum_get(source);
  transitions:
    each transiton is represented by one if/goto statement
    Example: goto matched_##matched##char_a;
  finish or accept:
    it it represented by return keyword, name is self-explanatory
    Example: return lunit_create(&lexme_info, TOK_PROCEDURE);

  Note that first state isn't a macro and has to be written by hand
  It is a switch statement that transfers control to one of these macros
*/ 
#define STATE_1(matched, char_a)  \
matched_##matched:                \
    PROLOGUE                      \
    TRANSITION_M(matched, char_a) \
    TRANSITION_F(id, following)   \
    EPILOGUE(TOK_ID)

#define STATE_2(matched, char_a, char_b) \
matched_##matched:                       \
    PROLOGUE                             \
    TRANSITION_M(matched, char_a)        \
    TRANSITION_M(matched, char_b)        \
    TRANSITION_F(id, following)          \
    EPILOGUE(TOK_ID)

#define STATE_F(matched, token) \
matched_##matched:              \
    PROLOGUE                    \
    TRANSITION_F(id, following) \
    EPILOGUE(token)

// TODO move it to config file
#define LEXME_ALLOCATION_STEP 32

struct lexme_info
{
    char *lexme;
    size_t line;
    size_t column;
    size_t index;
    size_t space;
};

typedef struct lexme_info lexme_info_t;

static inline int skip_comment(source_t *source)
{
    int c;

    /*
      Eat up chars until EOL/EOF
    */
    while (true)
    {

        c = rufum_get_char(source);

        // TODO
        if (c < 0);

        if (c == '\n' || c == SOURCE_END)
            break;
    }

    /*
      Unread newline/SOURCE_END
    */
    rufum_unget_char(source, c);

    return;
}

static inline enum status skip_multiline_comment(source_t *source)
{
    size_t depth;
    int c;

    /*
      We have already read one '{' in function skip
    */
    depth = 1;

    /*
      Loop until we don't leave all the comments
      We can enter a new comment by encountering '{'
      To leave it we must encounter '}'
      If we encounter SOURCE_END while
      in a comment we indicate a failure
    */
    while (true)
    {
        c = rufum_get_char(source);

        if (c == SOURCE_END)
            return FAILURE;
            
        if (c == '{')
            depth += 1;
        else if (c == '}')
            depth -= 1;

        if (depth == 0)
            return OK;
    }
}

static inline enum status try_skip_newline(source_t *source)
{
    size_t space_count;
    int c;

    /*
      Our task is to skip following sequence: '\n *\\' (regexp)
      We have already read the newline, now we have to
      skip all the spaces of the following line
      However we need to count them to be able to reverse the operation
      Let's loop until we find something that isn't a space
    */
    space_count = 0;

    while (true)
    {
        c = rufum_get_char(source);

        if (c != ' ')
            break;

        space_count += 1;
    }

    /*
      Backslash encountered? If so this is continuation of previous line
      Report that we successfully skipped EOL
    */
    if (c == '\\')
        return SUCCESS;

    /*
      We encountered something else, we have to unread everything
      Basically when we encounter a newline we don't know if
      next line is a continuation of it so we look ahead
      If it turns out that it isn't we need to revert to previous state
      This means unreading that character and spaces it was preceeded by
      This also means unreading that newline we read in skip()
      before we got called, finally report a failure
      Note: failure doesn't mean error, it means that
      we failed to skip a newline
    */
    size_t iter = space_count;

    rufum_unget_char(c);

    while (iter != 0)
    {
        rufum_unget_char(' ');
        iter -= 1;
    }

    rufum_unget_char('\n');

    return FAILURE;
}

static inline enum status skip(source_t *source, size_t *line_ptr,
    size_t *column_ptr)
{
    while (true)
    {
        int c;
        size_t line;
        size_t column;

        /*
          Save position of first character of... something
          It matters only if this something is a multiline comment
          However we can query position of a character
          before we read it not after, this is why we do it now
          Line and column numbers will be used for diagnostics
        */
        line = rufum_get_line(source);
        column = rufum_get_column(source);

        c = rufum_get_char(source);

        /*
          Skip spaces, comments and escaped newlines
          skip_multiline_comment can fail if it encounters
          a comment that spans all the way to EOF
          TODO rufum_get_char can fail
        */
        switch (c)
        {
            case ' ':
                continue;
            case '#':
                skip_comment(source);
                continue;
            case '{':
                if (skip_multiline_comment(source) == FAILURE)
                {
                    *line_ptr = line;
                    *column_ptr = column;

                    return;
                }
                continue;
            case '\n':
                if (try_skip_newline(source) == SUCCESS)
                    continue;
        }

        return OK;
    }
}

static inline bool test_char_downcase(int c)
{
    return c >= 'a' && c <= 'z';
}

static inline bool test_char_upcase(int c)
{
    return c >= 'A' && c <= 'Z';
}

static inline bool test_char_binary(int c)
{
    return c == '0' || c == '1';
}

static inline bool test_char_octal(int c)
{
    return c >= '0' && c <= '7';
}

static inline bool test_char_decimal(int c)
{
    return c >= '0' && c <= '9';
}

static inline bool test_char_hexadecimal(int c)
{
    bool rv;

    rv = test_char_decimal(c);
    rv = rv || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');

    return rv;
}

static inline bool test_char_binary_suffix(int c)
{
    bool rv;

    rv = test_char_downcase(c) || test_char_upcase(c);
    rv = rv || (c >= '2' && c <= '9') || c == '?' || c == '_';
}

static inline bool test_char_octal_suffix(int c)
{
    bool rv;

    rv = test_char_downcase(c) || test_char_upcase(c);
    rv = rv || c == '8' || c == '9' || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_sequence(int c)
{
    return c == '.' || c == ',';
}

static inline bool test_char_decimal_suffix(int c)
{
    bool rv;

    rv = test_char_downcase(c) || test_char_upcase(c);
    rv = rv || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_hexadecimal_suffix(int c)
{
    bool rv;

    rv = (c >= 'f' && c <= 'z') || (c >= 'F' && c <= 'Z');
    rv = rv || c == '?' || c == '_';

    return rv;
}

static inline bool test_char_following(int c)
{
    bool rv;

    rv = test_char_downcase(c) || test_char_upcase(c) || test_char_decimal(c);
    rv = rv || c == '?' || c == '_';

    return rv;
}

static enum status append_to_lexme(lexme_info_t *lexme_info, char c)
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
            return FAILURE;

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
    lemxe_info->text[lexme_info->index] = c;
    lexme_info->index += 1;

    return OK;
}

static lunit_t *create_lunit(lexme_info_t *lexme_info, enum token token)
{
    lunit_t *lunit;

    /*
      Our tast is to create a lunit (lexer unit)
      and populate it with information
      First we need to allocate it
    */
    lunit = malloc(sizeof(*lunit));

    if (lunit == NULL)
        return NULL;

    /*
      Allocate a structure to hold information about
      lexme's value and its length
      If we fail we also need to free lunit
      in order to prevent memory leaks
    */
    lunit->lexme = lstring_create();

    if (lunit->lexme == NULL)
    {
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
}

lunit_t *rufum_get_lunit(source_t *source)
{
    lexme_info_t lexme_info;

    /*
      We are going to realloc this so we have to initialize it to NULL
    */
    lexme_info.lexme = NULL;
    
    /*
      We haven't allocated any space for the lexme (space)
      and we haven't appended any characters yet (index)
    */
    lexme_info.index = 0;
    lexme_info.space = 0;

    /*
      Initialize line and column numbers to zero
      First of all this position is invalid
      There will never be a character at this position
      We do it so that skip() can report an error 
      by setting these two variables to nonzero value
    */
    lexme_info.line = 0;
    lexme_info.column = 0;

    /*
      Skip spaces, comments and escaped newlines
      If after calling skip() line and column numbers
      are nonzero then skip failed
      There is only one reason why skip can fail:
      it encountered a multiline comment that spanned
      all the way up to end of file
      This is the only case when we return something
      that should have been skipped as a token
      We do it because we want to report
      at which line that comment started
    */
    skip(source, &lexme_info.line, &lexme_info.column);

    if (line != 0)
        return make_lunit(lexme_info, TOK_BAD_COMMENT);

    /*
      Now that we have skipped all the things that
      shouldn't be returned as tokens
      we can finally start doing our job
      This is the first state of finite state machine
      Read one char an based on it move to a appropriate state
    */
    c = rufum_get_char(source);

    /*
      Neither this if statement nor this switch represent a state
      Their task is to move to an appropriate state, nothing more
    */
    if (test_char_downcase(c))
    {
        switch (c)
        {
            case 'p':
                goto matched_p;
            case 'r':
                goto matched_r;
            default:
                goto downcase;
        }
    }

    if (c == '0')
        goto initial_zero;

    if (test_char_decimal(c))
        goto decimal_int;

    if (c == '\n')
        return create_lunit(&lexme_info, TOK_EOL);

    if (c == EOF)
        return create_lunit(&lexme_info, TOK_EOF);

    return create_lunit(&lexme_info, TOK_UNKNOWN);



initial_zero:
    /*
      We have got a zero and this is first character read
      We need to destinguish initial_zero from decimal_int
      because zero can form 0b, 0o and 0x prefixes
      Otherwise it works just like decimal_int
    */
    PROLOGUE
    TRANSITION_F(decimal_int, decimal)
    TRANSITION_S(decimal_dot, '.')
    TRANSITION_S(decimal_comma, ',')
    TRANSITION_S(binary_prefix, 'b')
    TRANSITION_S(octal_prefix, 'o')
    TRANSITION_S(hexadecimal_prefix, 'x')
    EPILOGUE(TOK_DEC_INT)

decimal_int:
    /*
      Previous character was a digit but it is possible
      we have read one or more commas before that
      Following sequences of characters can lead here:
      '1', '2' ... '9', '369', '12,12', '0,3', '032' etc.
      Zero does not belong here, however this doesn't mean
      numbers starting with zero do not belong here
    */
    PROLOGUE
    TRANSITION_F(decimal_int, decimal)
    TRANSITION_S(decimal_int_dot, '.')
    TRANSITION_S(decimal_int_comma, ',')
    TRANSITION_F(decimal_int_suffix, decimal_suffix)
    EPILOGUE(TOK_DEC_INT)

decimal_int_dot:
    /*
      We have encountered a dot after sequence of digits and optionally
      one or more commas, previous character was a digit though
      If we encounter a digit then we have a float
      Example: We just got '0' after "1." which gives us "1.0"
      However if we get another dot or comma then we have an invalid sequence
    */
    PROLOGUE
    TRANSITION_S(decimal_float, decimal)
    TRANSITION_F(decimal_int_sequence, sequence)
    EPILOGUE(TOK_DEC_INT_DOT)

decimal_int_comma:
    PROLOGUE
    TRANSITION_F(decimal_int, decimal)
    TRANSITION_F(decimal_int_sequence, sequence)
    EPILOGUE(TOK_DEC_INT_COM)

decimal_int_sequence:
    PROLOGUE
    TRANSITION_F(decimal_int_sequence, following)
    EPILOGUE(TOK_DEC_INT_SEQ)

decimal_int_suffix:
    PROLOGUE
    TRANSITION_F(decimal_int_suffix, following)
    EPILOGUE(TOK_DEC_INT_SUF)

decimal_float:
    PROLOGUE
    EPILOGUE(TOK_DEC_FLT)
}
