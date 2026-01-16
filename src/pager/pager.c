#include "pager.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * pager_open - opens file and returns a pointer to the pager struct
 *
 * @arena: arena for storing the pager
 * @filename: name of the file to open
 *
 * Return: pointer to pager or NULL if could not open
 * */
Pager *pager_open (Arena *arena, const char *filename)
{
    int fd = open (filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);

    if (fd == -1)
    {
        return NULL;
    }

    off_t file_len = lseek (fd, 0, SEEK_END);
    Pager *pager = push_struct_zero (arena, Pager);
    pager->fd = fd;
    pager->file_len = (uint32_t) file_len;
    pager->num_pages = (file_len / PAGE_SIZE);

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pager->pages[i] = NULL;
    }

    if (file_len % PAGE_SIZE != 0)
    {
        printf (
            "Warning: File length is not a multiple of page size. Corrupt?n");
    }

    return pager;
}

/**
 * pager_open - opens file and returns a pointer to the page
 *
 * @arena: arena for storing the pager
 * @pager: contains pages
 * @page_num: page number of page to get
 *
 * Return: pointer to page
 * */
void *pager_get_page (Arena *arena, Pager *pager, uint32_t page_num)
{
    if (page_num >= TABLE_MAX_PAGES)
    {
        printf ("Error: Page number %d out of bounds for cache (Max %d).\n",
                page_num, TABLE_MAX_PAGES);
        exit (EXIT_FAILURE);
    }

    if (pager->pages[page_num] != NULL)
    {
        return pager->pages[page_num];
    }
    void *page = push_array_zero (arena, void, PAGE_SIZE);

    uint32_t num_pages = pager->file_len / PAGE_SIZE;

    if (pager->file_len % PAGE_SIZE)
    {
        num_pages += 1;
    }

    if (page_num < num_pages)
    {
        lseek (pager->fd, page_num * PAGE_SIZE, SEEK_SET);
        ssize_t bytes_read = read (pager->fd, page, PAGE_SIZE);
        if (bytes_read == -1)
        {
            printf ("Error reading file: %d\n", errno);
            exit (EXIT_FAILURE);
        }
    }

    pager->pages[page_num] = page;
    return page;
}

/**
 * pager_flush - writes page to disk
 * @pager: pointer to pager
 * @page_num: page number
 */
void pager_flush (Pager *pager, uint32_t page_num)
{
    if (pager->pages[page_num] == NULL)
    {
        printf ("Error: Tried to flush page %d which is not in cache.\n",
                page_num);
        exit (EXIT_FAILURE);
    }

    off_t offset = page_num * PAGE_SIZE;

    if (lseek (pager->fd, offset, SEEK_SET) == -1)
    {
        perror ("Error seeking in pager_flush");
        exit (EXIT_FAILURE);
    }

    ssize_t bytes_written =
        write (pager->fd, pager->pages[page_num], PAGE_SIZE);
    if (bytes_written == -1)
    {
        perror ("Error flushing page to disk");
        exit (EXIT_FAILURE);
    }

    uint32_t file_end = offset + bytes_written;
    if (file_end > pager->file_len)
    {
        pager->file_len = file_end;
        pager->num_pages = pager->file_len / PAGE_SIZE;
    }
}

void pager_close (Pager *pager)
{
    if (close (pager->fd) == -1)
    {
        perror ("Error closing db file");
        exit (EXIT_FAILURE);
    }
}
/**
 * pager_slotted_insert - Inserts a Key-Value pair into the node
 * @key: Pointer to the key (Table Name)
 * @key_size: Length of the key
 * @val: Pointer to the value (Serialized Schema)
 * @val_size: Length of the value
 */
bool pager_slotted_insert (void *node, void *key, uint32_t key_size, void *val,
                           uint32_t val_size)
{
    SlottedPageHeader *header = (SlottedPageHeader *) node;

    uint32_t total_payload = sizeof (uint32_t) + key_size + val_size;
    uint32_t slot_space = sizeof (Slot);

    // free space = Data Start - Slot Array End
    uint32_t used_upper =
        sizeof (SlottedPageHeader) + (header->num_cells * sizeof (Slot));
    uint32_t free_space = header->data_start - used_upper;

    if (free_space < (total_payload + slot_space))
    {
        return false; // Page Full
    }

    // heap Space - Grow Backwards
    header->data_start -= total_payload;
    uint8_t *heap_ptr = (uint8_t *) node + header->data_start;

    // Write[KeyLen] [Key] [Value]
    memcpy (heap_ptr, &key_size, sizeof (uint32_t));
    memcpy (heap_ptr + 4, key, key_size);
    memcpy (heap_ptr + 4 + key_size, val, val_size);

    // 4. Update Slot Directory
    Slot *slots = (Slot *) ((uint8_t *) node + sizeof (SlottedPageHeader));
    slots[header->num_cells].offset = header->data_start;
    slots[header->num_cells].size = total_payload;

    header->num_cells++;
    return true;
}

void slot_get_content (void *node, uint16_t slot_index, void **key_out,
                       uint32_t *key_len_out, void **val_out,
                       uint32_t *val_len_out)
{
    SlottedPageHeader *header = (SlottedPageHeader *) node;
    Slot *slots = (Slot *) ((uint8_t *) node + sizeof (SlottedPageHeader));
    Slot s = slots[slot_index];

    uint8_t *ptr = (uint8_t *) node + s.offset;

    uint32_t klen;
    memcpy (&klen, ptr, sizeof (uint32_t));

    *key_len_out = klen;
    *key_out = ptr + 4;

    *val_out = ptr + 4 + klen;

    *val_len_out = s.size - (4 + klen);
}
