
#include "lstatus.h"
#include "source.h"

#include <stdbool.h>
#include <stddef.h>

/*
  There are four functions that use these macros
  skip, skip_comment, skip_multiline_comment and try_skip_newline
*/

#define GET_CHAR                        \
    error = rufum_get_char(source, &c); \
    if (error != LEXER_OK)              \
        return error;

#define UNGET_CHAR(c) \
    error = rufum_unget_char(source, c); \
    if (error != LEXER_OK);              \
        return error;

static lstatus_t skip_comment(source_t *source)
{
    lstatus_t error;
    int c;

    /*
      Eat up chars until EOL/SOURCE_END
    */
    while (true)
    {
        /*
          Attempt to read one character
          If rufum_get_char fails pass the error to the caller
        */
        GET_CHAR

        /*
          Newline? This means we succesfully skipped a comment
          We just should unread it and we are done
          Result of this call is the return value
        */
        if (c == '\n')
            return rufum_unget_char(source, c);

        /*
          We reached the end while skipping the comment
          This means that the last line isn't terminated my EOL
          Report it to the caller
        */
        if (c == SOURCE_END)
            return LEXER_BAD_COMMENT;
    }
}

static lstatus_t skip_multiline_comment(source_t *source)
{
    lstatus_t error;
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
        /*
          Attempt to read one character, on failure return the error
        */
        GET_CHAR

        /*
          If this is the end then we report failure
        */
        if (c == SOURCE_END)
            return LEXER_BAD_MULTILINE_COMMENT;
        
        /*
          Modify depth if we enter/leave a multiline comment
        */
        if (c == '{')
            depth += 1;
        else if (c == '}')
            depth -= 1;

        /*
          Return once we leave the initial comment
        */
        if (depth == 0)
            return LEXER_OK;
    }
}

static lstatus_t try_skip_newline(source_t *source, bool *skipped)
{
    lstatus_t error;
    int c;
    size_t space_count;

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
        GET_CHAR

        if (c != ' ')
            break;

        space_count += 1;
    }

    /*
      Backslash encountered? If so this is continuation of previous line
      Report that we successfully skipped EOL
    */
    if (c == '\\')
    {
        *skipped = true;
        return LEXER_OK;
    }

    /*
      We encountered something else, we have to unread everything
      Basically when we encounter a newline we don't know if
      next line is a continuation of it so we look ahead
      If it turns out that it isn't we need to revert to previous state
      This means unreading that character and spaces it was preceeded by
      This also means unreading that newline we read in skip()
      before we got called, finally report we failed to skip newline
    */

    UNGET_CHAR(c)

    for (size_t iter = space_count; iter != 0; --iter)
        UNGET_CHAR(' ')

    UNGET_CHAR('\n')

    *skipped = false;
    return LEXER_OK;
}

static lstatus_t skip(source_t *source, lexme_info_t *lexme_info)
{
    while (true)
    {
        lstatus_t error;
        size_t line;
        size_t column;
        int c;
        bool skipped;

        /*
          Save position of first character of... something
          It matters only if this something is a comment
          We can query position of a character before
          we read it not after, this is why we do it now
          Line and column numbers will be used for diagnostics
        */
        line = rufum_get_line(source);
        column = rufum_get_column(source);

        /*
          Read one character, return error indicator on failure
        */
        GET_CHAR

        /*
          Skip spaces, comments and escaped newlines
          Functions skip_comment, skip_multiline_comment and try_skip_newline
          set variable error to indicate success or failure
        */
        switch (c)
        {
            case ' ':
                continue;
            case '#':
                error = skip_comment(source);

                /*
                  This is a special case where we save position of '#'
                  and tell the lexer that we encountered a comment
                  that was followed by SOURCE_END and not a newline
                */
                if (error == LEXER_BAD_COMMENT)
                {
                    lexme_info->line = line;
                    lexme_info->column = column;

                    return error;
                }

                /*
                  Simply fail if there was any other error
                */
                if (error != LEXER_OK)
                    return error;

                continue;
            case '{':
                error = skip_multiline_comment(source);

                /*
                  This is true if we encountered unterminated comment
                  We should tell the user where this comment started
                  Hence we save its position
                */
                if (error == LEXER_BAD_MULTILINE_COMMENT)
                {
                    lexme_info->line = line;
                    lexme_info->column = column;

                    return error;
                }

                /*
                  Simply fail if there was any other error
                */
                if (error != LEXER_OK)
                    return error;

                continue;
            case '\n':
                error = try_skip_newline(source, &skipped);

                /*
                  Fail on any error
                */
                if (error != LEXER_OK)
                    return error;

                /*
                  If try_skip_newline has set skipped to false then we
                  are positioned at the very newline we encountered earlier
                  Hence without this we would enter infinite loop on newline
                  As we would match this case statement again
                  We do not unread anything, this was done by try_skip_error()
                */
                if (skipped == false)
                    return LEXER_OK;
                else
                    continue;
        }

        /*
          Case statement didn't match anything, this is something else
          Unread this character and stop
        */
        UNGET_CHAR(c)

        return LEXER_OK;
    }
}
