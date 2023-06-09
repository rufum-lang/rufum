#ifndef RUFUM_PARSER_SOURCE_H
#define RUFUM_PARSER_SOURCE_H

#include "lstatus.h"

#include <common/status.h>

#include <stddef.h>
#include <stdio.h>

#define SOURCE_END 256
#define SOURCE_MEMORY_ERROR -1
#define SOURCE_LINE_LIMIT_ERROR -2
#define SOURCE_COLUMN_LIMIT_ERROR -3

typedef struct source source_t;

source_t *rufum_new_file_source(FILE *fd);

source_t *rufum_new_string_source(char const * const string);

void rufum_destroy_source(source_t *source);

lstatus_t rufum_unget_char(source_t *source, int c);

lstatus_t rufum_get_char(source_t *source, int *char_ptr);

size_t rufum_get_line(source_t *source);

size_t rufum_get_column(source_t *source);

#endif
