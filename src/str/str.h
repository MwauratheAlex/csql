#ifndef STR_H
#define STR_H

#include "../arena/arena.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *str;
    size_t len;
} str8;

#define STR_FMT(s)  (int) ((s).len), (char *) ((s).str)
#define str8_lit(s) (str8){(uint8_t *) (s), sizeof (s) - 1}

str8 str8_from_range (uint8_t *start, uint8_t *end);
str8 str8_from_cstr (const char *s);

bool str8_match (str8 a, str8 b, bool ignore_case);
#define str8_equals(a, b) str8_match (a, b, false)

str8 str8_copy (Arena *a, str8 s);

#endif /* STR_H */
