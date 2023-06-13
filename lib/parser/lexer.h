#ifndef RUFUM_PARSER_LEXER_H
#define RUFUM_PARSER_LEXER_H

#include "lstatus.h"
#include "lunit.h"
#include "source.h"

void rufum_destroy_lunit(lunit_t *lunit);
lstatus_t rufum_scan(lunit_t **lunit_ptr, source_t *source);

#endif
