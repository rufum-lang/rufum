#ifndef RUFUM_PARSER_SOURCE_H
#define RUFUM_PARSER_SOURCE_H

#include "lstatus.h"

#include <common/status.h>

#include <external/rufum.h>

#include <stddef.h>
#include <stdio.h>

#define SOURCE_END -1

typedef struct source source_t;

source_t *rufum_new_file_source(rufum_reader_fn reader, void *data);

source_t *rufum_new_string_source(char const * const string);

void rufum_destroy_source(source_t *source);

lstatus_t rufum_unget_char(source_t *source, int c);

lstatus_t rufum_get_char(source_t *source, int *char_ptr);

size_t rufum_get_line(source_t *source);

size_t rufum_get_column(source_t *source);

#endif
