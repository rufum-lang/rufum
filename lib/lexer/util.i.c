#include "lstatus.h"

#include <common/memory.h>

#include <stdlib.h>

#define LEXME_ALLOCATION_STEP 32

static void lexme_initialize(lexme_info_t *lexme_info, source_t *source)
{
    /*
      We are going to realloc it so we have to initialize it to NULL
    */
    lexme_info->text = NULL;

    /*
      We haven't allocated any space for the lexme (space)
      and we haven't appended any characters yet (index)
    */
    lexme_info->index = 0;
    lexme_info->space = 0;

    /* 
      Get location of current lexme
    */
    lexme_info->line = rufum_get_line(source);
    lexme_info->column = rufum_get_column(source);

    return;
}

static lstatus_t lexme_append(lexme_info_t *lexme_info, char c)
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
          every LEXME_ALLOCATION_STEP characters added
          lexme_info->text is initialized to NULL by lexme_initialize
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

static void lexme_empty(lexme_info_t *lexme_info)
{
    /*
      Function free checks if its argument is NULL, we are safe
    */
    free(lexme_info->text);

    return;
}

static lunit_t *create_lunit(lexme_info_t *lexme_info, token_t token)
{
    /*
      Our task is to create a lunit (lexer unit)
      and populate it with information
      First we need to allocate it
    */
    lunit_t *lunit;

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
      Allocate a structure to hold information
      about lexme's value and length
      If we fail we also need to free lunit and
      new_text in order to prevent memory leaks
    */
    lunit->lexme = rufum_lstr_from_bytes(new_text, lexme_info->index);

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

static lstatus_t handle_error(lexme_info_t *lexme_info, lstatus_t error)
{
    /*
      This is a wrapper function, it exists so that we can write:
        if (error != LEXER_OK)
            return handle_error(&lexme_info, error);
      See? No curly brackets. It makes code easier to follow
      It is possible that text is NULL at this point
      but free does check if its argument is NULL
    */
    free(lexme_info->text);

    return error;
}


static lstatus_t handle_skip_error(lunit_t **lunit_ptr,
    lexme_info_t *lexme_info, lstatus_t error)
{
    /*
      This happens when we encounter a oneline comment
      not terminated by a newline (terminated by end of input)
      In this case we return a special lunit
    */
    if (error == LEXER_BAD_COMMENT)
    {
        lunit_t *lunit;

        lunit = create_lunit(lexme_info, TOK_BAD_COM);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;

        return LEXER_OK;
    }

    /*
      We encountered an unterminated multiline comment
      In this case we rerturn a special lunit
    */
    if (error == LEXER_BAD_MULTILINE_COMMENT)
    {
        lunit_t *lunit;

        lunit = create_lunit(lexme_info, TOK_BAD_ML_COM);

        if (lunit == NULL)
            return LEXER_MEMORY_ERROR;

        *lunit_ptr = lunit;

        return LEXER_OK;
    }

    /*
      We return all other errors
      At this point nothing has been appended to the lexme
      No need to free lexme_info->text
    */
    return error;
}
