#include "source.h"

#include <common/memory.h>
#include <common/status.h>

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

typedef enum source_type
{
    SOURCE_FILE,
    SOURCE_STRING
} source_type_t;

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

static status_t save_column(source_t *source)
{
    /*
      First resize the stack to make space for the new element
    */
    size_t *new_stack;
    size_t new_size;

    new_size = (source->column_stack_index + 1) * sizeof(*new_stack);
    new_stack = realloc(source->column_stack, new_size);
    
    if (new_stack == NULL)
        return FAILURE;

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
    
    return OK;
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

static int move_forward(source_t *source, int c)
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
            return SOURCE_LINE_LIMIT_ERROR;

        /*
          There is only one thing that ccan cause
          save_column to fail - lack of memory
        */
        if (save_column(source) == FAILURE)
            return SOURCE_MEMORY_ERROR;

        source->line += 1;
        source->column = 1;
    }
    else
    {
        if (source->column == SIZE_MAX)
            return SOURCE_COLUMN_LIMIT_ERROR;

        source->column += 1;
    }

    /*
      All the error codes we return are negative
      Hence return 0 on success
    */
    return 0;
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

status_t rufum_unget_char(source_t *source, int c)
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
        return OK;
    }

    /*
      Reallocate stack to make space for one more character
    */
    size_t new_size;
    char *new_stack;

    new_size = source->unread_stack_index + 1;
    new_stack = realloc(source->unread_stack, new_size);
    
    if (new_stack == NULL)
        return FAILURE;

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

    return OK;
}

static int reread(source_t *source)
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
        return SOURCE_MEMORY_ERROR;

    source->unread_stack = new_stack;

    /*
      Update line/column number
    */
    int rv = move_forward(source, c);

    /*
      If move_forward failed return that error, else return the character
      Function move_forward returns 0 on success and negative int on failure
      There is no conflict as all character values are >= 0
    */
    return (rv < 0) ? rv : c;
}

static int read_from_file(source_t *source)
{
    /*
      Read one character
    */
    int c;

    c = fgetc(source->source.fd);

    /*
      If we encounter EOF we return a NULL byte
      There is no next character so we don't change
      line number nor column number
      Note thet we can't return EOF because
      its value varies between implementations
      TODO
    */
    if (c == EOF)
        return SOURCE_END;

    /*
      Update line/column number
    */
    int rv = move_forward(source, c);

    /*
      If move_forward failed return that error, else return the character
      Function move_forward returns 0 on success and negative int on failure
      There is no conflict as all character values are >= 0
    */
    return (rv < 0) ? rv : c;
}

static int read_from_string(source_t *source)
{
    /*
      Check if we have have reached end of string
      If se return SOURCE_END
      TODO
    */
    if (source->buffer_index == source->buffer_size)
        return SOURCE_END;
    /*
      Read one character from the buffer incrementing the index variable
    */
    int c;

    c = source->source.buffer[source->buffer_index - 1];
    source->buffer_index += 1;

    /*
      Update line/column number
    */
    int rv = move_forward(source, c);

    /*
      If move_forward failed return that error, else return the character
      Function move_forward returns 0 on success and negative int on failure
      There is no conflict as all character values are >= 0
    */
    return (rv < 0) ? rv : c;
}

int rufum_get_char(source_t *source)
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
        return SOURCE_END;
    }
    else if (source->unread_stack_index != 0)
    {
        return reread(source);
    }
    else if (source->type == SOURCE_FILE)
    {
        return read_from_file(source);
    }
    else /* source->type == SOURCE_STRING */
    {
        return read_from_string(source);
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
