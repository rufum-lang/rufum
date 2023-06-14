#include "list.h"
#include "msg.h"

#include <common/lstring.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 2048

static int trim_chunk(char **chunk_ptr, size_t size)
{
    /*
      If we have read zero bytes free the chunk
    */
    if (size == 0)
    {
        free(*chunk_ptr);

        return 0;
    }

    /*
      Trim chunk of memory pointed to by chunk_ptr
      We use new_chunk variable to avoid memory leaks
    */
    char *new_chunk;

    new_chunk = realloc(*chunk_ptr, size);

    if (new_chunk == NULL)
    {
        free(*chunk_ptr);

        return 1;
    }

    /*
      Update caller's copy of chunk
    */
    *chunk_ptr = new_chunk;

    return 0;
}

static lstring_t *read_chunk(FILE *fd)
{
    char *chunk;
    size_t bytes_read;

    /*
      Allocate space for our buffer
    */
    chunk = malloc(CHUNK_SIZE);

    if (chunk == NULL)
        return NULL;

    /*
      Read entire chunk worth of data
    */
    bytes_read = fread(chunk, 1, CHUNK_SIZE, fd);

    /*
      There are two reasons as to why we read less than expected
      Either there was a I/O error or we encountered EOF
    */
    if (bytes_read != CHUNK_SIZE)
    {
        /*
          Check for I/O error
        */
        if (ferror(fd))
        {
            puts("I/O error occurred while reading input file");
            free(chunk);

            return NULL;
        }

        /*
          If it wasn't I/O error then we encountered EOF
          We have read less than CHUNK_SIZE bytes
          but buffer is CHUNK_SIZE bytes long
          This means we would waste space if we
          don't reallocate it, hence we trim it
          This function can free chunk and set it to NULL
          if we have read zero bytes, why waste space?
        */
        int rv = trim_chunk(&chunk, bytes_read);

        /*
          Non-zero value means memory allocation error
          When trim_chunk returns error it frees
          the chunk so we don't have to do it here
        */
        if (rv != 0)
        {
            puts(MEMORY_ERROR_MSG);

            return NULL;
        }
    }

    /*
      Allocate lstring, do it even when chunk is NULL
      Returning lstring with length == 0 (and therefore
      text == NULL) is our way of saying "we encountered EOF"
    */
    lstring_t *lstring;

    lstring = lstring_from_bytes(chunk, bytes_read);

    if (lstring == NULL)
    {
        puts(MEMORY_ERROR_MSG);
        free(chunk);

        return NULL;
    }

    return lstring;
}

static char *read(FILE *fd)
{
    lstring_list_t list;

    list.array = NULL;
    list.count = 0;

    /*
      Here we read the file chunk by chunk
      Every chunk is represented by a lstring data structure
      Once we read a chunk we append it to a list
      We do this because reallocing big buffer is very inefficient
      Later on we are going to concatenate the list
    */
    while (true)
    {
        lstring_t *lstring;
        int rv;

        /*
          Read a chunk of data as lstring_t
          NULL lstring means thet we encountered an error
          It is up to read_chunk to print it
        */
        lstring = read_chunk(fd);

        if (lstring == NULL)
        {
            list_empty(&list);

            return NULL;
        }

        /*
          Empty lstring means EOF, we just destroy it
          as there is no need to append empty lstring to the list
          We stop looping as there is nothing to read after EOF
        */
        if (lstring->length == 0)
        {
            lstring_destroy(lstring);

            break;
        }

        /*
          Append that lstring to the list, zero means success
        */
        rv = list_append(&list, lstring);

        if (rv != 0)
        {
            list_empty(&list);
            lstring_destroy(lstring);

            return NULL;
        }
    }

    /*
      Now concatenate the lstrings into a single string
    */
    char *buffer;

    buffer = list_concat(&list);

    list_empty(&list);

    return buffer;
}

char *read_file(char *file_name)
{
    /*
      Open file that contains already processed source code
    */
    FILE *fd;

    fd = fopen(file_name, "r");

    if (fd == NULL)
    {
        perror("Couldn't open input file");

        return NULL;
    }

    /*
      Read contents of that file, no that isn't that read
      It prints error messages so we son't have to
    */
    char *buffer;

    buffer = read(fd);
    fclose(fd);

    return buffer;
}

