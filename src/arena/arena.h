#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SIZE_MB (1024 * 1024)

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

typedef enum
{

    ArenaFlag_NoZero = 0,
    ArenaFlag_Zero = 1
} ArenaFlag;

void arena_init (Arena *a, void *backing_buffer, size_t backing_buffer_length);
void arena_free_all (Arena *a);
void *arena_alloc (Arena *a, size_t size, ArenaFlag zero);
void *arena_resize (Arena *a, void *old_memory, size_t old_size,
                    size_t new_size, ArenaFlag zero);
Temp_Arena_Memory temp_arena_memory_begin (Arena *a);
void temp_arena_memory_end (Temp_Arena_Memory temp);

#define push_array_zero(arena, type, count)                                    \
    (type *) arena_alloc (arena, sizeof (type) * (count), ArenaFlag_Zero)

#define push_array_no_zero(arena, type, count)                                 \
    (type *) arena_alloc (arena, sizeof (type) * (count), ArenaFlag_NoZero)

#define push_struct_zero(arena, type)                                          \
    (type *) arena_alloc (arena, sizeof (type), ArenaFlag_Zero)

#endif /* ARENA_H */
