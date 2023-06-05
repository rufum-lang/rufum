#ifndef RUFUM_COMMON_LSTRING_H
#define RUFUM_COMMON_LSTRING_H

#include "status.h"

#include <stdio.h>

typedef struct
{
    char *text; /* not null terminated */
    size_t length;
} lstring_t;

lstring_t *lstring_create(void);

lstring_t *lstring_from_bytes(char *bytes, size_t size);

void lstring_destroy(lstring_t *lstring);

status_t lstring_append_char(lstring_t *lstring, char c);

status_t lstring_append_string(lstring_t *lstring, char *str);

status_t lstring_append_lstring(lstring_t *dest, lstring_t *src);

//void lstring_append_size(lstring_t *lstring, size_t size);

void lstring_reverse(lstring_t *lstring);

//void lstring_print(lstring_t *lstring, FILE *fd);

#endif
