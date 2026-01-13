#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    unsigned char *buf;
    size_t buf_len;
    size_t prev_offset;
    size_t curr_offset;
} Arena;

typedef struct
{
    Arena *arena;
    size_t prev_offset;
    size_t curr_offset;
} Temp_Arena_Memory;

void
arena_init (Arena *a, void *backing_buffer, size_t backing_buffer_length);

void
arena_free_all (Arena *a);

void *
arena_alloc (Arena *a, size_t size);

void *
arena_resize (Arena *a, void *old_memory, size_t old_size, size_t new_size);

#endif /* ARENA_H */
