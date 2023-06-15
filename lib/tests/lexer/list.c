#include "list.h"

#include <common/lstring.h>

#include <stdlib.h>

int list_append(lstring_list_t *list, lstring_t *lstring)
{
    lstring_t **new_array;
    size_t new_size;

    /*
      Calculete new size of the array and then reallocate it
    */
    new_size = (list->count + 1) * sizeof(*new_array);
    new_array = realloc(list->array, new_size);

    /*
      Indicate failure if we failed to expand the array
    */
    if (new_array == NULL)
        return -1;

    /*
      list->array has been moved or is the same as new_array
      so it is safe to do this, there are no memory leaks
    */
    list->array = new_array;

    /*
      Insert the new element and indicate successfull completion of the task
    */
    list->array[list->count] = lstring;
    list->count += 1;

    return 0;
}

void list_empty(lstring_list_t *list)
{
    /*
      Free all members of the list
      We do not free the list itself as it is allocated on the stack
      Array size (list->count) used as its subscript points one element
      past array's end and this is why we don't free element at this index
    */
    for (size_t iter = 0; iter != list->count; ++iter)
        rufum_lstr_destroy(list->array[iter]);
}

char *list_concat(lstring_list_t *list)
{
    size_t sum = 0;

    /*
      Sum lengths of all the lstrings
      list->count is size of the array and array's size used as its
      subscripts points one element past the end of the array
      We don't want to read past the end of array and
      this is why we stop when iter equals list->count
    */
    for (size_t iter = 0; iter != list->count; ++iter)
        sum += list->array[iter]->length;

    /*
      Allocate string big enought to hold all the data
      We add one because we want to terminate the string with NULL byte
    */
    char *string;

    string = malloc(sum + 1);

    /*
      Copy data from lstrings to the string
      Variable i is used to index the list
      Variable j is used to index lstring in the list
      Variable ix is used to index the destination string
    */
    size_t ix = 0;

    for (size_t i = 0; i != list->count; ++i)
    {
        for (size_t j = 0; j != list->array[i]->length; ++j)
        {
            string[ix] = list->array[i]->text[j];
            ix += 1;
        }
    }

    /*
      Terminate the string with NULL byte
      Example:
        list->count = 2;
        list->array[0] = "Hello " (len = 6)
        list->array[1] = "world\n" (len = 6)
        string = "Hello world\n" (len = 12)
        ix after last iteration = 12
        character indices = 0 .. 11 inclusive
      Where do we put the NULL byte? At index 12 (ix)
    */
    string[ix] = '\0';

    return string;
}
