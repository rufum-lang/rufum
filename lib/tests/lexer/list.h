#ifndef RUFUM_TESTS_LEXER_LIST_H
#define RUFUM_TESTS_LEXER_LIST_H

#include <common/lstring.h>

#include <stddef.h>

struct lstring_list
{
    lstring_t **array;
    size_t count;
};

typedef struct lstring_list lstring_list_t;



int list_append(lstring_list_t *list, lstring_t *lstring);

void list_empty(lstring_list_t *list);

char *list_concat(lstring_list_t *list);

#endif
