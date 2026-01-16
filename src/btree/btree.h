#ifndef BTREE_H
#define BTREE_H
#include "../db/db.h"
#include "../pager/pager.h"

#include <stdint.h>

typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

/*
 * NODE HEADER FORMAT (Common to all nodes)
 * ----------------------------------------
 * | NodeType (1) | IsRoot (1) | ParentPointer (4) | ...
 */
#define NODE_TYPE_SIZE        sizeof (uint8_t)
#define NODE_TYPE_OFFSET      0
#define IS_ROOT_SIZE          sizeof (uint8_t)
#define IS_ROOT_OFFSET        (NODE_TYPE_SIZE)
#define PARENT_POINTER_SIZE   sizeof (uint32_t)
#define PARENT_POINTER_OFFSET (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE                                                \
    (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

/*
 * LEAF NODE HEADER FORMAT
 * -----------------------
 * | Common Header | NumCells (4) | ...
 */
#define LEAF_NODE_NUM_CELLS_SIZE   sizeof (uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET (COMMON_NODE_HEADER_SIZE)
#define LEAF_NODE_NEXT_LEAF_SIZE   sizeof (uint32_t)
#define LEAF_NODE_NEXT_LEAF_OFFSET                                             \
    (LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE)
#define LEAF_NODE_HEADER_SIZE                                                  \
    (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE                        \
     + LEAF_NODE_NEXT_LEAF_SIZE)

/*
 * LEAF NODE BODY FORMAT
 * ---------------------
 * | Header | Cell[0] | Cell[1] | ... | Cell[N] |
 * * A Cell contains: [ Key (4 bytes) | Row Data (FIXED SIZE) ]
 */
#define LEAF_NODE_KEY_SIZE sizeof (uint32_t)

// Fixed row size.
#define LEAF_NODE_VALUE_SIZE      256
#define LEAF_NODE_CELL_SIZE       (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS       (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)

void initialize_leaf_node (void *node);
uint32_t *leaf_node_num_cells (void *node);
void *leaf_node_cell (void *node, uint32_t cell_num);
uint32_t *leaf_node_key (void *node, uint32_t cell_num);
void *leaf_node_value (void *node, uint32_t cell_num);
NodeType get_node_type (void *node);
void set_node_root (void *node, uint8_t is_root);
uint32_t *leaf_node_next_leaf (void *node);
int btree_find_key (Database *db, uint32_t root_page_num, void *key,
                    uint32_t key_len, void **out_page);

#endif /* BTREE_H */
