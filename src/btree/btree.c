#include "btree.h"

#include <stdint.h>
#include <string.h>

void initialize_leaf_node (void *node)
{
    SlottedPageHeader *header = (SlottedPageHeader *) node;
    header->node_type = NODE_LEAF;
    header->is_root = 0;
    header->num_cells = 0;
    header->next_leaf = 0;

    header->data_start = PAGE_SIZE;
}

uint32_t *leaf_node_next_leaf (void *node)
{
    return (uint32_t *) (node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

NodeType get_node_type (void *node)
{
    uint8_t *n = (uint8_t *) node;
    return (NodeType) n[NODE_TYPE_OFFSET];
}

void set_node_root (void *node, uint8_t is_root)
{
    uint8_t *n = (uint8_t *) node;
    n[IS_ROOT_OFFSET] = is_root;
}

uint32_t *leaf_node_num_cells (void *node)
{
    uint8_t *n = (uint8_t *) node;
    return (uint32_t *) (n + LEAF_NODE_NUM_CELLS_OFFSET);
}

void *leaf_node_cell (void *node, uint32_t cell_num)
{
    uint8_t *n = (uint8_t *) node;
    return n + LEAF_NODE_HEADER_SIZE + (cell_num * LEAF_NODE_CELL_SIZE);
}

uint32_t *leaf_node_key (void *node, uint32_t cell_num)
{
    return (uint32_t *) leaf_node_cell (node, cell_num);
}

void *leaf_node_value (void *node, uint32_t cell_num)
{
    uint8_t *cell = (uint8_t *) leaf_node_cell (node, cell_num);
    return cell + LEAF_NODE_KEY_SIZE;
}

int btree_find_key (Database *db, uint32_t root_page_num, void *key,
                    uint32_t key_len, void **out_page)
{
    void *page = pager_get_page (db->global_arena, db->pager, root_page_num);
    SlottedPageHeader *header = (SlottedPageHeader *) page;

    // TODO Handle internal nodes
    Slot *slots = (Slot *) ((uint8_t *) page + sizeof (SlottedPageHeader));

    // TODO Binary Search
    for (int i = 0; i < header->num_cells; i++)
    {
        if (slots[i].size == 0)
        {
            continue; // skip deleted
        }

        void *slot_key;
        uint32_t slot_klen;
        void *slot_val;
        uint32_t slot_vlen;

        slot_get_content (page, i, &slot_key, &slot_klen, &slot_val,
                          &slot_vlen);

        if (key_len == slot_klen && memcmp (key, slot_key, key_len) == 0)
        {
            if (out_page)
            {
                *out_page = page;
            }
            return i;
        }
    }

    if (out_page)
    {
        *out_page = page;
    }

    return -1;
}
