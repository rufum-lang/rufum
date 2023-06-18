#include "contents.h"
#include "list.h"
#include "msg.h"

#include <common/lstring.h>
#include <lexer/lexer.h>
#include <lexer/lstatus.h>
#include <lexer/lunit.h>
#include <lexer/source.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static int reader(char *buffer, void *data, size_t *size)
{
    /*
      Argument data is our file
    */
    FILE *fd = (FILE *) data;

    /*
      Check if we reached end of file
      If we did return zero to indicate success
      and set size to 0 to indicate EOF
    */
    if (feof(fd))
    {
        *size = 0;

        return 0;
    }

    /*
      Read requested amount of one byte chunks
    */
    size_t bytes_read;

    bytes_read = fread(buffer, 1, *size, fd);

    /*
      If we read all the bytes successfully return 0 to indicate success
      Before that tell the caller how many bytes we successfully read
    */
    if (bytes_read == *size)
    {
        *size = bytes_read;

        return 0;
    }

    /*
      We read less bytes than we expected
      It could be because we encountered EOF
      or it could be an error
      Check whether EOF is the case
    */
    if (feof(fd))
    {
        *size = bytes_read;

        return 0;
    }

    /*
      I/O error occurred
    */
    return -1;
}


#define CASE(token) case token: return #token;

static char *token_to_string(token_t token)
{
    switch (token)
    {
        CASE(TOK_LOWERCASE)
        CASE(TOK_UPPERCASE)

        CASE(TOK_BIN_INT)
        CASE(TOK_OCT_INT)
        CASE(TOK_DEC_INT)
        CASE(TOK_HEX_INT)

        CASE(TOK_BIN_INT_DOT)
        CASE(TOK_OCT_INT_DOT)
        CASE(TOK_DEC_INT_DOT)
        CASE(TOK_HEX_INT_DOT)

        CASE(TOK_BIN_INT_COM)
        CASE(TOK_OCT_INT_COM)
        CASE(TOK_DEC_INT_COM)
        CASE(TOK_HEX_INT_COM)

        CASE(TOK_BIN_INT_SEQ)
        CASE(TOK_OCT_INT_SEQ)
        CASE(TOK_DEC_INT_SEQ)
        CASE(TOK_HEX_INT_SEQ)

        CASE(TOK_BIN_INT_SUF)
        CASE(TOK_OCT_INT_SUF)
        CASE(TOK_DEC_INT_SUF)
        CASE(TOK_HEX_INT_SUF)

        CASE(TOK_BIN_FLT)
        CASE(TOK_OCT_FLT)
        CASE(TOK_DEC_FLT)
        CASE(TOK_HEX_FLT)

        CASE(TOK_BIN_FLT_DOT)
        CASE(TOK_OCT_FLT_DOT)
        CASE(TOK_DEC_FLT_DOT)
        CASE(TOK_HEX_FLT_DOT)

        CASE(TOK_BIN_FLT_COM)
        CASE(TOK_OCT_FLT_COM)
        CASE(TOK_DEC_FLT_COM)
        CASE(TOK_HEX_FLT_COM)

        CASE(TOK_BIN_FLT_SEQ)
        CASE(TOK_OCT_FLT_SEQ)
        CASE(TOK_DEC_FLT_SEQ)
        CASE(TOK_HEX_FLT_SEQ)

        CASE(TOK_BIN_FLT_SUF)
        CASE(TOK_OCT_FLT_SUF)
        CASE(TOK_DEC_FLT_SUF)
        CASE(TOK_HEX_FLT_SUF)

        CASE(TOK_EOL)
        CASE(TOK_END)

        CASE(TOK_BAD_COM)
        CASE(TOK_BAD_ML_COM)

        CASE(TOK_UNKNOWN)

        default: return "INVALID";
    }
}

/*
  Meaning of this frmat string:
    Variadic arguments:
      1st: token to print (%s)
      2nd: line number (%zu)
      3rd: column number (%zu)
      4th: lexme length (%zu)
      5th and 6th: length of a lexme and the lexme (%.*s)
        (we do not print fifth argument)
*/
#define FORMAT_STRING "%s: pos=(%zu, %zu), len=%zu\n | "

static char *write_lunit_normal(lunit_t *lunit, size_t *size_ptr)
{
    /*
      Convert token to its string representation
      Note that token_string is a string literal. We aren't responsible
      for freeing it. In fact we can't free it. It's in .rodata
      Rodata is a read only section of the executable
    */
    char *token_string;

    token_string = token_to_string(lunit->token);

    /*
      Well, this is pretty complex... Let me explain
      According to C99 when second argument is zero then nothing is printed
      Instead this call returns numbers of characters it would print if second
      argument was big enough but NOT counting terminating NULL byte
      We use this to learn how big buffer we need
    */
    size_t size;

    size = snprintf(NULL, 0, FORMAT_STRING, token_string,
        lunit->line, lunit->column, lunit->lexme->length);

    /*
      We want to append lunit->lexme->text manually
      We must ensure there is enought space for that
      We also want to append a newline
      Save current size as this is where we will append it
      Note that we can't use "%.*s" as it expects an int and not size_t
    */
    size_t manual_append_index = size;
    size += lunit->lexme->length + 1;

    /*
      Allocate buffer big enough to store the result string
    */
    char *buffer;

    buffer = malloc(size);

    if (buffer == NULL)
        return NULL;

    /*
      Write the string. Variable size is larger then string we write
      so we need not worry about buffer overruns
      The string is going to be null terminated but we will overwrite it
    */
    snprintf(buffer, size, FORMAT_STRING, token_string,
        lunit->line, lunit->column, lunit->lexme->length);

    /*
      Manually append lunit->lexme->text
      This also overwrites null byte (which is good)
    */
    strncpy(&buffer[manual_append_index], lunit->lexme->text,
        lunit->lexme->length);

    /*
      Add newline
      Array's size used as its subscript points one element past the last one
      We want to set the last one to newline, hence we subtract 1 from size
    */
    buffer[size - 1] = '\n';

    *size_ptr = size;
    return buffer;
}

/*
  This is here to silence warnings
*/
#undef FORMAT_STRING

#define FORMAT_STRING "%s: pos=(%zu, %zu), len=%zu\n | 0x%X (hex)\n"

static char *write_lunit_hex(lunit_t *lunit, size_t *size_ptr)
{
    /*
      Convert token to its string representation
      Note that token_string is a string literal. We aren't responsible
      for freeing it. In fact we can't free it. It's in .rodata
      Rodata is a read only section of the executable
    */
    char *token_string;

    token_string = token_to_string(lunit->token);

    /*
      Well, this is pretty complex... Let me explain
      According to C99 when second argument is zero then nothing is printed
      Instead this call returns numbers of characters it would print if second
      argument was big enough but NOT counting terminating NULL byte
      We use this to learn how big buffer we need
    */
    size_t size;

    size = snprintf(NULL, 0, FORMAT_STRING, token_string,
        lunit->line, lunit->column, lunit->lexme->length,
        (unsigned int) lunit->lexme->text[0]);

    /*
      Allocate buffer big enough to store the result string
    */
    char *buffer;

    buffer = malloc(size + 1);

    if (buffer == NULL)
        return NULL;

    /*
      Write the string. We need to add one to size because otherwise
      last character would get trimmed. The string is going to be
      null terminated but we will ignore the null byte
      That is we return size + 1 character long string but we report
      that we returned size character long string
    */
    snprintf(buffer, size + 1, FORMAT_STRING, token_string,
        lunit->line, lunit->column, lunit->lexme->length,
        (unsigned int) lunit->lexme->text[0]);

    *size_ptr = size;
    return buffer;
}

static char *write_lunit_end(lunit_t *lunit, size_t *size_ptr)
{
    /*
      Convert token to its string representation
      Note that token_string is a string literal. We aren't responsible
      for freeing it. In fact we can't free it. It's in .rodata
      Rodata is a read only section of the executable
    */
    char *token_string;

    token_string = token_to_string(lunit->token);

    /*
      Copy token_string to a newly allocated buffer
      We can't return token_string as retured string has to be free-able
      Allocate space for the string but also make space for a newline
      This is why we add one to size
    */
    size_t length, size;
    char *buffer;

    length = strlen(token_string);
    size = length + 1;
    buffer = malloc(length);

    if (buffer == NULL)
        return NULL;

    /*
      Copy the string to the buffer and append a newline
      Array's size used as its subscript points one element past the last one
      We want to insert newline at the last element so we should subtract one
      form size. However length is equal to size - 1 and this is why we use it
      After all length = size + 1 (see above)
    */
    strncpy(buffer, token_string, length);
    buffer[length] = '\n';

    *size_ptr = size;
    return buffer;
}

static lstring_t *lunit_to_lstring(lunit_t *lunit)
{
    char *string;
    size_t size;

    /*
      Here we convert lunit to its textual representation
      There are three cases we need to consider:
        - TOK_END: end of input does not have a representation since
          this isn't a character, it doesn't have length or position either
          All we want to do is to write the token we encountered
          There is a special function that does it: write_lunit_eof
        - TOK_EOL: end of line (or newline) it's whitespace and not visible
          in a terminal and this is why we print it as hexadecimal value
        - TOK_UNKNOWN: an unknown character, we print the token, position and
          length. What about the lexme? Well, there are two cases to consider:
          - character is printable - we use write_character_normal
            to show the character as is
          - character isn't printable - we use write_character_hex
            to show character as hexadecimal number
            Examples: DELETE and ESCAPE characters
        - everything else: we just print the token, the lexme as well as
          its position and its length
    */
    if (lunit->token == TOK_END)
    {
        string = write_lunit_end(lunit, &size);
    }
    else if (lunit->token == TOK_EOL)
    {
        string = write_lunit_hex(lunit, &size);
    }
    else if (lunit->token == TOK_UNKNOWN)
    {
        char character = lunit->lexme->text[0];

        /*
          Check whether character is printable
        */
        if (character < 0x20 || character >= 0x7f)
            string = write_lunit_hex(lunit, &size);
        else
            string = write_lunit_normal(lunit, &size);
    }
    else /* Everything else */
    {
        string = write_lunit_normal(lunit, &size);
    }

    if (string == NULL)
        return NULL;

    /*
      Now create the actual lstring, if we fail free the buffer (memory leaks)
    */
    lstring_t *lstring;

    lstring = rufum_lstr_from_bytes(string, size);

    if (lstring == NULL)
    {
        free(string);

        return NULL;
    }

    return lstring;
}

static void print_error(lstatus_t error)
{
    switch (error)
    {
        case LEXER_MEMORY_ERROR:
            puts(MEMORY_ERROR_MSG);
            return;
        case LEXER_IO_ERROR:
            puts("An I/O error occured");
            return;
        case LEXER_LINE_LIMIT_ERROR:
            puts("File is too long to scan");
            return;
        case LEXER_COLUMN_LIMIT_ERROR:
            puts("Line is too long to scan");
            return;
        default:
            puts("print_error: Unknown error (please report this)");
            return;
    }
}

static char *scan(source_t *source)
{
    /*
      Initialize list so that it's ready to be reallocated
    */
    lstring_list_t list;

    list.array = NULL;
    list.count = 0;

    /*
      Convert lunits to their textual representation until
      an error occurs or end of input is encountered
    */
    bool stop = false;

    do {
        lunit_t *lunit;
        lstatus_t error;

        /*
          Read one lunit (lexer unit), if an error occurs then
          if the error was caused by a unexpected end of input
          then don't treat is as an error just mark this iteration
          as the last one, else print an error message
        */
        error = rufum_scan(&lunit, source);

        /*
          Check if the comment was terminated by unexpected end of input
        */
        if (error == LEXER_BAD_COMMENT || error == LEXER_BAD_MULTILINE_COMMENT)
        {
            error = LEXER_OK;
            stop = true;
        }

        /*
          We print all the other errors and return
        */
        if (error != LEXER_OK)
        {
            print_error(error);
            list_empty(&list);

            return NULL;
        }

        /*
          We are going to destroy lunit so we need to check if it's TOK_END
          If it is true then this is the last iteration
        */
        if (lunit->token == TOK_END)
            stop = true;

        /*
          Now convert lunit to a lstring, then discard it
        */
        lstring_t *lstring;

        lstring = lunit_to_lstring(lunit);
        rufum_destroy_lunit(lunit);

        if (lstring == NULL)
        {
            puts(MEMORY_ERROR_MSG);
            list_empty(&list);

            return NULL;
        }

        /*
          Attempt to append lstring to the list, 0 means we succeeded
        */
        int rv = list_append(&list, lstring);

        if (rv != 0)
        {
            puts(MEMORY_ERROR_MSG);
            list_empty(&list);
            rufum_lstr_destroy(lstring);

            return NULL;
        }

    } while (stop == false);

    /*
      Concatenate elements of the list to get a string
      Whether we succeed or not list is no longer needed
    */
    char *string = list_concat(&list);
    list_empty(&list);

    return string;
}

char *scan_file(char *file_name)
{
    /*
      Open file that contains source code to scan
    */
    FILE *fd;

    fd = fopen(file_name, "r");

    if (fd == NULL)
    {
        perror("Couldn't open source file");

        return NULL;
    }

    /*
      Create source, a structure that holds information about input source
      Use the file variant. Note that fd is used internally by source
      We can close the file after destroying the source
    */
    source_t *source;

    source = rufum_new_file_source(reader, (void *) fd);

    if (source == NULL)
    {
        puts(MEMORY_ERROR_MSG);
        fclose(fd);

        return NULL;
    }

    /*
      Scan contents of file TODO meaningless comment
      scan() prints error messages so we don't have to
    */
    char *buffer;

    buffer = scan(source);
    rufum_destroy_source(source);
    fclose(fd);

    return buffer;
}

char *scan_string(char *file_name)
{
    /*
      Open file that contains source code to scan
    */
    FILE *fd;

    fd = fopen(file_name, "r");

    if (fd == NULL)
    {
        perror("Couldn't open source file");

        return NULL;
    }

    /*
      Read contents of a file we are going to use as a source
    */
    char *string;

    string = get_file_contents(fd);
    fclose(fd);

    /*
      Create source, a structure that holds information about input source
      Use the string variant
    */
    source_t *source;

    source = rufum_new_string_source(string, strlen(string));

    if (source == NULL)
    {
        puts(MEMORY_ERROR_MSG);

        return NULL;
    }

    /*
      Get contents of a file TODO meaningless comment
      scan() prints error messages so we don't have to
    */
    char *buffer;

    buffer = scan(source);
    rufum_destroy_source(source);

    return buffer;
}
