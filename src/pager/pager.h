#ifndef PAGER_H
#define PAGER_H

#include "../arena/arena.h"
#include "stdint.h"

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE       4096
#define TABLE_MAX_PAGES 100

/**
 * PAGE STRUCTURE
 *   0                                                offset    ofset + size
 *  ------------------------------------------------------------------------
 *  | [slots(offset, size)] | ........................| data    | ..........
 *  ------------------------------------------------------------------------
 */

typedef struct
{
    uint16_t offset;
    uint16_t size;
} Slot;

typedef struct
{
    uint8_t node_type; // LEAF or INTERNAL
    uint8_t is_root;
    uint16_t num_cells;  // Number of active slots
    uint16_t data_start; // Offset to data
    uint16_t next_leaf;  // next page number
} SlottedPageHeader;

typedef struct
{
    int fd;
    uint32_t file_len;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

Pager *pager_open (Arena *arena, const char *filename);
void *pager_get_page (Arena *arena, Pager *pager, uint32_t page_num);
void pager_flush (Pager *pager, uint32_t page_num);
void pager_close (Pager *pager);
bool pager_slotted_insert (void *node, void *key, uint32_t key_size, void *val,
                           uint32_t val_size);
void slot_get_content (void *node, uint16_t slot_index, void **key_out,
                       uint32_t *key_len_out, void **val_out,
                       uint32_t *val_len_out);

#endif /* PAGER_H */
