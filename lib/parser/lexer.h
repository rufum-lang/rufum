#ifndef RUFUM_PARSER_LEXER_H
#define RUFUM_PARSER_LEXER_H

#include "lstatus.h"
#include "lunit.h"
#include "source.h"

lstatus_t rufum_get_lunit(lunit_t **lunit_ptr, source_t *source);

#endif
