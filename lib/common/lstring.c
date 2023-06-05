#include "lstring.h"

#include "status.h"
//#include <convert/to_lstring.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

lstring_t *lstring_create(void)
{
    lstring_t *lstring;

    /*
      Allocate the structure
    */
    lstring = malloc(sizeof(*lstring));

    if (lstring == NULL)
        return NULL;

    /*
      Initialize the structure, we pass text to realloc
      and as such we must set it to NULL
    */
    lstring->length = 0;
    lstring->text = NULL;

    return lstring;
}

lstring_t *lstring_from_bytes(char *bytes, size_t size)
{
    lstring_t *lstring;

    /*
      Allocate the structure
    */
    lstring = malloc(sizeof(*lstring));

    if (lstring == NULL)
        return NULL;

    /*
      Initialize the structure
    */
    lstring->length = size;
    lstring->text = bytes;

    return lstring;
}

void lstring_destroy(lstring_t *lstring)
{
    /*
      Destroy lstring, if lstring->text is NULL free does nothing
    */
    free(lstring->text);
    free(lstring);

    return;
}

status_t lstring_append_char(lstring_t *lstring, char c)
{
    size_t new_size;
    char *new_text;

    /*
      We need to extend lstring->text by one byte
      realloc when supplied with NULL as first
      argument works just like malloc
    */
    new_size = lstring->length + 1;
    new_text = realloc(lstring->text, new_size);

    if (new_text == NULL)
        return FAILURE;

    lstring->text = new_text;
    /*
      Append string to lstring->text, space is allocated already
      If we use length of an array as its subscript it's going to
      point one byte after its end
      This is where we want to insert this char
    */
    lstring->text[lstring->length] = c;
    lstring->length += 1;

    return OK;
}

status_t lstring_append_string(lstring_t *lstring, char *string)
{
    size_t string_length;
    size_t new_size;
    char *new_text;

    string_length = strlen(string);

    /*
      Reallocate buffer to accomodate new string
      When you invoke realloc with NULL as first
      argument it works like malloc
    */
    new_size = lstring->length + string_length;
    new_text = realloc(lstring->text, new_size);

    if (new_text == NULL)
        return FAILURE;

    lstring->text = new_text;

    /*
      Append string to lstring->text, space is allocated already
      If we use length of an array as its subscript it's going to
      point one byte after its end
      This is where we want to append this string
      & operator makes a pointer out of this position
      We use strncpy because log->text isn't null terminated
    */
    strncpy(&lstring->text[lstring->length], string, string_length);
    lstring->length += string_length;

    return OK;
}

status_t lstring_append_lstring(lstring_t *dest, lstring_t *src)
{
    size_t new_size;
    char *new_text;

    /*
      Reallocate buffer to accomodate new string
      When you invoke realloc with NULL as first
      argument it works like malloc
    */
    new_size = dest->length + src->length;
    new_text = realloc(dest->text, new_size);

    if (new_text == NULL)
        return FAILURE;

    dest->text = new_text;

    /*
      Append string to dest->text, space is allocated already
      If we use length of an array as its subscript it's going to
      point one byte after its end
      This is where we want to append this string
      & operator makes a pointer out of this position
      We use strncpy because dest->text isn't null terminated
    */
    strncpy(&dest->text[dest->length], src->text, src->length);
    dest->length += src->length;

    return OK;
}

/*
void lstring_append_size(lstring_t *lstring, size_t size)
{
    struct lstring *size_as_lstr;

    size_as_lstr = size_to_lstring(size);

    lstring_append_lstring(lstring, size_as_lstr);

    lstring_destroy(size_as_lstr);
}
*/

void lstring_reverse(lstring_t *lstring)
{
    size_t left_index, right_index, end_index;

    /*
      We want to start at the very beginning
    */
    left_index = 0;

    /*
      Length of a given array used as its subscript points
      one element past the last element of an array
      We want to start from the last element, hence -1
    */
    right_index = lstring->length - 1;

    /*
      We want to stop when left_index is equal to end_index
      Example 1: even length
        lstring->text = "aaabbb"
        lstring->length = 6
        end_index = 6/2 = 3
        0 != end_index so we swap 0 and 5
        1 != end_index so we swap 1 and 4
        2 != end_index so we swap 2 and 3
        3 == end_index so we stop
      Example 2: odd length
        lstring->text "aaaxbbb"
        lstring->length = 7
        end_index = 7/2 = 3
        0 != end_index so swap 0 and 6
        1 != end_index so swap 1 and 5
        2 != end_index so swap 2 and 4
        3 == end_index so stop
    */
    end_index = lstring->length / 2;

    while (left_index != end_index)
    {
        /*
          First we need to read two characters, left and right
          If we read character from the left and assign it to the right
          then we lose the right character and vice versa
        */
        char left_char, right_char;

        left_char = lstring->text[left_index];
        right_char = lstring->text[right_index];

        /*
          Swap the characters
        */
        lstring->text[left_index] = right_char;
        lstring->text[right_index] = left_char;

        /*
          Move to next characters in the buffer
        */
        left_index += 1;
        right_index -= 1;
    }

    return;
}

/*
void lstring_print(struct lstring *lstring, FILE *fd)
{
    size_t elements_written;

    // Write contents of the log all at once, not byte by byte

    elements_written = fwrite(lstring->text, lstring->length, 1, fd);

    CHECK_IO_ERROR(elements_written != 1)
}
*/
