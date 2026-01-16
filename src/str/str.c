#include "str.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool char_is_upper (uint8_t c);
uint8_t lower_from_char (uint8_t c);

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

bool char_is_upper (uint8_t c)
{
    return ('A' <= c && c <= 'Z');
}

uint8_t lower_from_char (uint8_t c)
{
    if (char_is_upper (c))
    {
        c += ('a' - 'A');
    }
    return c;
}

bool str8_match (str8 a, str8 b, bool ignore_case)
{
    if (a.len != b.len)
        return false;

    if (a.str == b.str)
    {
        return true;
    }

    if (!ignore_case)
    {
        return memcmp (a.str, b.str, a.len) == 0;
    }

    for (uint64_t i = 0; i < a.len; ++i)
    {
        uint8_t la = lower_from_char (a.str[i]);
        uint8_t lb = lower_from_char (b.str[i]);
        if (la != lb)
        {
            return false;
        }
    }

    return true;
}

int str8_compar (str8 a, str8 b, bool ignore_case)
{
    int cmp = 0;
    uint64_t size = a.len < b.len ? a.len : b.len;

    if (ignore_case)
    {
        for (uint64_t i = 0; i < size; ++i)
        {
            uint8_t la = lower_from_char (a.str[i]);
            uint8_t lb = lower_from_char (b.str[i]);
            if (la < lb)
            {
                cmp = -1;
                break;
            }
            else if (la > lb)
            {
                cmp = +1;
                break;
            }
        }
    }
    else
    {
        for (uint64_t i = 0; i < size; ++i)
        {
            if (a.str[i] < b.str[i])
            {
                cmp = -1;
                break;
            }
            else if (a.str[i] > b.str[i])
            {
                cmp = +1;
                break;
            }
        }
    }

    if (cmp == 0)
    {
        // shorter prefix must precede longer prefixes
        if (a.len > b.len)
        {
            cmp = +1;
        }
        else if (b.len > a.len)
        {
            cmp = -1;
        }
    }

    return cmp;
}

str8 str8_copy (Arena *a, str8 s)
{
    str8 str;
    str.len = s.len;
    str.str = push_array_no_zero (a, uint8_t, s.len + 1);
    memcpy (str.str, s.str, s.len);
    str.str[str.len] = 0;
    return str;
}
