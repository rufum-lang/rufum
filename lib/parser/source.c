#include "source.h"

#include "lstatus.h"

#include <common/memory.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
  TODO Move it somewhere
  Here we save column number of encountered newline
  We do it so that when we unget a newline we can restore its position
  Example:
    We have a newline at line 2 column 10
    and we move one char forward to line 3 column 1
    Then we unget that newline moving back to line 2
    But what column number was that newline?
    We don't know! This is why we save it
*/

enum source_type
{
    SOURCE_FILE,
    SOURCE_STRING
};

typedef enum source_type source_type_t;

/*
  Typedef is in source.h
*/
struct source
{
    union
    {
        FILE *fd;
        char const *buffer;
    } source;
    char *unread_stack;
    size_t *column_stack;
    size_t line;
    size_t column;
    size_t unread_stack_index;
    size_t column_stack_index;
    size_t buffer_index;
    size_t buffer_size;
    source_type_t type;
    bool end;
};

static void init_source(source_t *source)
{
    source->unread_stack = NULL;
    source->column_stack = NULL;
    source->line = 1;
    source->column = 1;
    source->unread_stack_index = 0;
    source->column_stack_index = 0;
    source->end = false;

    return;
}

source_t *rufum_new_file_source(FILE *fd)
{
    source_t *new_source;

    NEW(new_source)

    if (new_source == NULL)
        return NULL;

    init_source(new_source);

    new_source->source.fd = fd;
    new_source->type = SOURCE_FILE;

    return new_source;
}

source_t *rufum_new_string_source(char const * const string)
{
    source_t *new_source;

    NEW(new_source)
    
    if (new_source == NULL)
        return NULL;

    init_source(new_source);

    new_source->source.buffer = string;
    new_source->buffer_index = 0;
    new_source->type = SOURCE_STRING;

    return new_source;
}

void rufum_destroy_source(source_t *source)
{
    /*
      Function free checks if its argument is NULL
    */
    free(source->unread_stack);
    free(source->column_stack);
    free(source);

    return;
}

static lstatus_t save_column(source_t *source)
{
    /*
      First resize the stack to make space for the new element
    */
    size_t *new_stack;
    size_t new_size;

    new_size = (source->column_stack_index + 1) * sizeof(*new_stack);
    new_stack = realloc(source->column_stack, new_size);
    
    if (new_stack == NULL)
        return LEXER_MEMORY_ERROR;

    source->column_stack = new_stack;

    /*
      Push current column number onto the stack
      Array's size used as its subscript points one element past the last one
      However stack has been expanded by one element already
      but the index variable hasn't been updated
      This means that by using it as a subscript we will access last element
      which is exactly where we want to insert the new element
      Finally update index variable
    */
    source->column_stack[source->column_stack_index] = source->column;
    source->column_stack_index += 1;
    
    return LEXER_OK;
}

static size_t restore_column(source_t *source)
{
    /*
      Array's size used as its subscript
      points one element past the last one
      Having this in mind to get the last element
      we need to subtract one from the subscript
      We also have to decrement the index
      since we are removing one element
      Why not do that before indexing the array?
      Note: We aren't downsizing the stack because
      we are going to push the element back eventually
    */
    source->column_stack_index -= 1;
    return source->column_stack[source->column_stack_index];
}

static lstatus_t move_forward(source_t *source, int c)
{
    /*
      Here we update line number and column number
      If chacter we just read is a newline then we save its position
      Next step is to increment line number and reset column number
      We do this because line number and column number tell us
      the position of next character we are going to read
      Since this character is preceeded by a newline
      it is on the next line compared to current character
      If the character we just read isn't a newline we move to next column

      I'm not sure whether checking if line/column number reached
      SIZE_MAX as I don't think file can have more than SIZE_MAX chars
      But it's better to be safe than sorry
      We exit because it would require extensive modification
      of the lexer, this function would no longer return
      void and that would make it way more complex
      And I'm not sure if this condition is ever met
      I wish I could just remove inner ifs #removing-code-is-good
      -- Mapniv
      TODO Add more useful error messages maybe?
    */
    if (c == '\n')
    {
        if (source->line == SIZE_MAX)
            return LEXER_LINE_LIMIT_ERROR;

        /*
          There is only one thing that can cause
          save_column to fail - lack of memory
        */
        if (save_column(source) == LEXER_MEMORY_ERROR)
            return LEXER_MEMORY_ERROR;

        source->line += 1;
        source->column = 1;
    }
    else
    {
        if (source->column == SIZE_MAX)
            return LEXER_COLUMN_LIMIT_ERROR;

        source->column += 1;
    }

    /*
      Indicate success
    */
    return LEXER_OK;
}

static void move_backward(source_t *source, int c)
{
    /*
      Move one character back, if we end up at newline then
      we decrement line number and then restore column number
      We saved the column number when we first encountered that newline
      Else decrement column number since we didn't move to previous line
    */
    if (c == '\n')
    {
        source->line -= 1;
        source->column = restore_column(source);
    }
    else
    {
        source->column -= 1;
    }

    return;
}

lstatus_t rufum_unget_char(source_t *source, int c)
{
    /*
      We are trying to push EOF, but this won't do
      as unread_stack stores chars not ints
      Storing ints would waste so much memory...
      Instead we set a flag, when reading chars we
      check if it is set, if so we return EOF
      This only allows to unread one EOF
      And we can't unread anything after unreading EOF
      as you will get EOF first and then the char!
    */
    if (c == SOURCE_END)
    {
        source->end = true;
        return LEXER_OK;
    }

    /*
      Reallocate stack to make space for one more character
    */
    size_t new_size;
    char *new_stack;

    new_size = source->unread_stack_index + 1;
    new_stack = realloc(source->unread_stack, new_size);
    
    if (new_stack == NULL)
        return LEXER_MEMORY_ERROR;

    source->unread_stack = new_stack;

    /*
      Push the character onto the stack
      incrementing number of items on the stack
      or in other words the stack index
      Variable unread_stack is an array of chars
      Making it an array of ints would be wasteful
    */
    source->unread_stack[source->unread_stack_index] = (char) c;
    source->unread_stack_index += 1;

    /*
      Update line and column number TODO
    */
    move_backward(source, c);

    return LEXER_OK;
}

static lstatus_t reread(source_t *source, int *char_ptr)
{
    /*
      Array's size used as its subscript points one element past the last one
      We want to get the last one and as such we subtract one
    */
    int c;

    c = source->unread_stack[source->unread_stack_index - 1];

    /*
      We removed one element from the stack
      Decrement unread_index and resize unread_stack
      based on unread_index (new size of the stack)
    */
    char *new_stack;

    source->unread_stack_index -= 1;
    new_stack = realloc(source->unread_stack, source->unread_stack_index);

    if (new_stack == NULL)
        return LEXER_MEMORY_ERROR;

    source->unread_stack = new_stack;

    /*
      Update line/column number
    */
    lstatus_t rv = move_forward(source, c);

    /*
      On failure pass the error to the caller
    */
    if (rv != LEXER_OK)
        return rv;

    /*
      Put c at the location pointed to by char_ptr
      so that the caller can access that value
      and return value that indicates that we completed our task
    */
    *char_ptr = c;
    return LEXER_OK;
}

static lstatus_t read_from_file(source_t *source, int *char_ptr)
{
    /*
      Read one character
    */
    int c;

    c = fgetc(source->source.fd);

    /*
      If we encounter EOF we return SOURCE_END
      There is no next character so we don't change
      line number nor column number
      Note thet we can't return EOF because
      its value varies between implementations
      TODO
    */
    if (c == EOF)
    {
        /*
          If error indicator is set for stream ferror returns non-zero
        */
        if (ferror(source->source.fd) != 0)
            return LEXER_IO_ERROR;
        
        *char_ptr = SOURCE_END;
        return LEXER_OK;
    }

    /*
      Update line/column number
    */
    lstatus_t rv = move_forward(source, c);

    /*
      On failure pass the error to the caller
    */
    if (rv != LEXER_OK)
        return rv;

    /*
      Put c at the location pointed to by char_ptr
      so that the caller can access that value
      and return value that indicates that we completed our task
    */
    *char_ptr = c;
    return LEXER_OK;
}

static lstatus_t read_from_string(source_t *source, int *char_ptr)
{
    /*
      Check if we have have reached end of string
      If se return SOURCE_END
      TODO
    */
    if (source->buffer_index == source->buffer_size)
    {
        *char_ptr = SOURCE_END;
        return LEXER_OK;
    }
    /*
      Read one character from the buffer incrementing the index variable
    */
    int c;

    c = source->source.buffer[source->buffer_index - 1];
    source->buffer_index += 1;

    /*
      Update line/column number
    */
    lstatus_t rv = move_forward(source, c);

    /*
      If there was an error pass returned value to the caller
    */
    if (rv != LEXER_OK)
        return rv;
    
    /*
      Put c at the location pointed to by char_ptr
      so that the caller can access that value
      and return value that indicates that we completed our task
    */
    *char_ptr = c;
    return LEXER_OK;
}

lstatus_t rufum_get_char(source_t *source, int *char_ptr)
{
    /*
      If we have unread a EOF return EOF
      Read first comment in rufum_unget for information on source->eof
      Else if we have unread any other chars return the one on the top
      Otherwise the behaviour depends on the source
      If it is a file source read next character from file
      If it is a string source read next character from buffer
    */
    if (source->end == true)
    {
        source->end = false;
        *char_ptr = SOURCE_END;
        return LEXER_OK;
    }
    else if (source->unread_stack_index != 0)
    {
        return reread(source, char_ptr);
    }
    else if (source->type == SOURCE_FILE)
    {
        return read_from_file(source, char_ptr);
    }
    else /* source->type == SOURCE_STRING */
    {
        return read_from_string(source, char_ptr);
    }
}

size_t rufum_get_line(source_t *source)
{
    return source->line;
}

size_t rufum_get_column(source_t *source)
{
    return source->column;
}
