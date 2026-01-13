#include "str.h"

#include <string.h>

str8 str8_from_range (uint8_t *start, uint8_t *end)
{
    str8 s;
    s.str = start;
    s.len = (size_t) (end - start);
    return s;
}

str8 str8_from_cstr (const char *s)
{
    str8 res;
    res.str = (uint8_t *) s;
    res.len = strlen (s);
    return res;
}

bool str8_equals (str8 a, str8 b)
{
    if (a.len != b.len)
        return false;
    return memcmp (a.str, b.str, a.len) == 0;
}
