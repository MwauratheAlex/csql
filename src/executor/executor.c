#include "executor.h"

#include "../arena/arena.h"
#include "../btree/btree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static ExecuteResult execute_create_table (Statement *stmt, Database *db);
static ExecuteResult execute_create_index (Statement *stmt, Database *db);
static ExecuteResult execute_insert (Statement *stmt, Database *db);
static ExecuteResult execute_select (Statement *stmt, Database *db,
                                     int client_fd);
static ExecuteResult execute_delete (Statement *stmt, Database *db);
static ExecuteResult execute_update (Statement *stmt, Database *db);
static bool row_matches_predicate (Table *t, void *row_data, int target_col_idx,
                                   void *target_val);

ExecuteResult execute_statement (Statement *s, Database *db, int client_fd)
{
    switch (s->type)
    {
    case STMT_CREATE_TABLE:
        return execute_create_table (s, db);
    case STMT_CREATE_INDEX:
        return execute_create_index (s, db);
    case STMT_SELECT:
        return execute_select (s, db, client_fd);
    case STMT_INSERT:
        return execute_insert (s, db);
    case STMT_UPDATE:
        return execute_update (s, db);
    case STMT_DELETE:
        return execute_delete (s, db);
    default:
        return EXECUTE_FAIL;
    }
}

static ExecuteResult execute_create_table (Statement *stmt, Database *db)
{
    if (db_find_table (db, stmt->create.table_name) != NULL)
    {
        return EXECUTE_TABLE_EXISTS;
    }

    uint32_t new_root_page = db->pager->num_pages++;

    void *data_node =
        pager_get_page (db->global_arena, db->pager, new_root_page);
    initialize_leaf_node (data_node);
    set_node_root (data_node, 1);

    pager_flush (db->pager, new_root_page);

    Table table = {0};
    table.root_page_num = new_root_page;
    table.col_count = stmt->create.col_count;

    for (int i = 0; i < table.col_count; i++)
    {
        table.columns[i] = stmt->create.columns[i];
    }

    uint8_t schema_blob[PAGE_SIZE];
    uint32_t blob_size = serialize_table (&table, schema_blob);

    void *catalog_root = pager_get_page (db->global_arena, db->pager, 0);

    bool success = pager_slotted_insert (
        catalog_root, stmt->create.table_name.str, stmt->create.table_name.len,
        schema_blob, blob_size);

    if (!success)
    {
        return EXECUTE_DB_FULL;
    }

    if (db->table_count < MAX_TABLES)
    {
        Table *t = push_struct_zero (db->global_arena, Table);
        t->table_name = str8_copy (db->global_arena, stmt->create.table_name);
        t->root_page_num = new_root_page;
        t->pager = db->pager;
        t->col_count = table.col_count;

        for (int i = 0; i < t->col_count; i++)
        {
            t->columns[i] = stmt->create.columns[i];
            t->columns[i].name =
                str8_copy (db->global_arena, stmt->create.columns[i].name);
        }

        db->tables[db->table_count++] = t;
    }
    else
    {
        return EXECUTE_TABLE_FULL;
    }

    pager_flush (db->pager, 0);
    db->pager->file_len = db->pager->num_pages * PAGE_SIZE;

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_create_index (Statement *stmt, Database *db)
{
    unsigned char local_buffer[PAGE_SIZE];
    Arena local_arena;
    arena_init (&local_arena, local_buffer, sizeof (local_buffer));

    Table *t = db_find_table (db, stmt->create_index.table_name);
    if (!t)
    {
        return EXECUTE_TABLE_EXISTS;
    }

    int col_idx = -1;
    for (int i = 0; i < t->col_count; i++)
    {
        if (str8_match (t->columns[i].name, stmt->create_index.col_name, true))
        {
            col_idx = i;
            break;
        }
    }

    if (col_idx == -1)
    {
        return EXECUTE_COL_NOT_FOUND;
    }
    if (db->index_count >= MAX_INDEXES)
    {
        return EXECUTE_DB_FULL;
    }

    Index *idx = &db->indexes[db->index_count++];
    idx->index_name =
        str8_copy (db->global_arena, stmt->create_index.index_name);
    idx->table_name =
        str8_copy (db->global_arena, stmt->create_index.table_name);
    idx->col_name = str8_copy (db->global_arena, stmt->create_index.col_name);
    idx->root_page_num = db->pager->num_pages++;

    void *idx_root =
        pager_get_page (db->global_arena, db->pager, idx->root_page_num);
    initialize_leaf_node (idx_root);
    set_node_root (idx_root, 1);
    pager_flush (db->pager, idx->root_page_num);

    void *table_root =
        pager_get_page (db->global_arena, db->pager, t->root_page_num);
    SlottedPageHeader *header = (SlottedPageHeader *) table_root;
    Slot *slots =
        (Slot *) ((uint8_t *) table_root + sizeof (SlottedPageHeader));

    int pk_idx = table_find_primary_key_index (t);
    if (pk_idx == -1)
    {
        pk_idx = 0;
    }

    for (int i = 0; i < header->num_cells; i++)
    {
        if (slots[i].size == 0)
        {
            continue;
        }

        Temp_Arena_Memory scratch = temp_arena_memory_begin (&local_arena);

        void *key, *val;
        uint32_t klen, val_len;
        slot_get_content (table_root, i, &key, &klen, &val, &val_len);

        str8 row_strings[MAX_COLUMNS];
        deserialize_row_to_strings (t, val, row_strings, &local_arena);

        str8 index_key = row_strings[col_idx];
        str8 pk_val = row_strings[pk_idx];

        pager_slotted_insert (idx_root, index_key.str, index_key.len,
                              pk_val.str, pk_val.len);

        temp_arena_memory_end (scratch);
    }

    pager_flush (db->pager, idx->root_page_num);
    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_insert (Statement *stmt, Database *db)
{
    Table *t = db_find_table (db, stmt->insert.table_name);
    if (!t)
    {
        return EXECUTE_TABLE_NOT_EXISTS;
    }

    if (stmt->insert.val_count != t->col_count)
    {
        return EXECUTE_TABLE_COL_COUNT_MISMATCH;
    }

    int pk_idx = table_find_primary_key_index (t);
    if (pk_idx == -1)
    {
        pk_idx = 0; // if pk table has no pk, pk=first column;
    }

    void *key_ptr = NULL;
    uint32_t key_len = 0;
    uint32_t id_key;

    if (t->columns[pk_idx].type == TYPE_INT)
    {
        char temp[32];
        snprintf (temp, 32, "%.*s", STR_FMT (stmt->insert.values[pk_idx]));
        id_key = atoi (temp);

        key_ptr = &id_key;
        key_len = sizeof (uint32_t);
    }
    else // TYPE_TEXT - validated earlier by the parser
    {
        key_ptr = stmt->insert.values[pk_idx].str;
        key_len = stmt->insert.values[pk_idx].len;
    }

    if (btree_find_key (db, t->root_page_num, key_ptr, key_len, NULL) != -1)
    {
        return EXECUTE_DUPLICATE_KEY;
    }

    uint8_t row_buffer[PAGE_SIZE];
    uint32_t row_size = serialize_row (t, stmt->insert.values, row_buffer);

    void *root_node =
        pager_get_page (db->global_arena, db->pager, t->root_page_num);

    if (!pager_slotted_insert (root_node, key_ptr, key_len, row_buffer,
                               row_size))
    {
        return EXECUTE_TABLE_FULL;
    }

    pager_flush (db->pager, t->root_page_num);

    for (int i = 0; i < db->index_count; i++)
    {
        Index *idx = &db->indexes[i];
        if (str8_match (idx->table_name, t->table_name, true))
        {
            int target_col_idx = table_find_col_index (t, idx->col_name);
            if (target_col_idx == -1)
            {
                continue;
            }

            str8 idx_key_val = stmt->insert.values[target_col_idx];

            str8 pk_val_str = stmt->insert.values[pk_idx];

            void *idx_page = pager_get_page (db->global_arena, db->pager,
                                             idx->root_page_num);

            pager_slotted_insert (idx_page, idx_key_val.str, idx_key_val.len,
                                  pk_val_str.str, pk_val_str.len);

            pager_flush (db->pager, idx->root_page_num);
        }
    }

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_select (Statement *stmt, Database *db,
                                     int client_fd)
{
    unsigned char local_buffer[PAGE_SIZE];
    Arena local_arena;
    arena_init (&local_arena, local_buffer, sizeof (local_buffer));

    Table *t1 = db_find_table (db, stmt->select.table_name);
    if (!t1)
        return EXECUTE_TABLE_NOT_EXISTS;

    Table *t2 = NULL;
    if (stmt->select.has_join)
    {
        t2 = db_find_table (db, stmt->select.join_table);
        if (!t2)
            return EXECUTE_TABLE_NOT_EXISTS;
    }

    typedef struct
    {
        Table *t;
        int idx;
    } ColMap;
    ColMap output_cols[MAX_COLUMNS];
    int output_count = 0;

    if (stmt->select.field_count == 0)
    {
        for (int i = 0; i < t1->col_count; i++)
            output_cols[output_count++] = (ColMap) {t1, i};
        if (t2)
            for (int i = 0; i < t2->col_count; i++)
                output_cols[output_count++] = (ColMap) {t2, i};
    }
    else
    {
        for (int i = 0; i < stmt->select.field_count; i++)
        {
            Table *target_t;
            int target_idx;
            if (resolve_column (t1, t2, stmt->select.fields[i], &target_t,
                                &target_idx)
                == -1)
                return EXECUTE_COL_NOT_FOUND;
            output_cols[output_count++] = (ColMap) {target_t, target_idx};
        }
    }

    Index *use_index = NULL;
    if (stmt->select.has_where && !stmt->select.has_join)
    {
        for (int i = 0; i < db->index_count; i++)
        {
            if (str8_match (db->indexes[i].index_name, t1->table_name, true)
                && str8_match (db->indexes[i].col_name,
                               stmt->select.where_column.col_name, true))
            {
                use_index = &db->indexes[i];
                break;
            }
        }
    }

    if (use_index)
    {
        void *idx_page = pager_get_page (db->global_arena, db->pager,
                                         use_index->root_page_num);
        SlottedPageHeader *idx_h = (SlottedPageHeader *) idx_page;
        Slot *idx_slots =
            (Slot *) ((uint8_t *) idx_page + sizeof (SlottedPageHeader));

        str8 search_key = stmt->select.where_value;

        for (int k = 0; k < idx_h->num_cells; k++)
        {
            if (idx_slots[k].size == 0)
                continue;
            void *ik, *iv;
            uint32_t ikl, ivl;
            slot_get_content (idx_page, k, &ik, &ikl, &iv, &ivl);

            if (ikl == search_key.len && memcmp (ik, search_key.str, ikl) == 0)
            {
                void *t1_page = pager_get_page (db->global_arena, db->pager,
                                                t1->root_page_num);
                SlottedPageHeader *t1_h = (SlottedPageHeader *) t1_page;

                for (int m = 0; m < t1_h->num_cells; m++)
                {
                    void *rk, *rv;
                    uint32_t rkl, rvl;
                    slot_get_content (t1_page, m, &rk, &rkl, &rv, &rvl);

                    if (rkl == ivl && memcmp (rk, iv, rkl) == 0)
                    {
                        Temp_Arena_Memory print_scratch =
                            temp_arena_memory_begin (&local_arena);

                        str8 row_vals[MAX_COLUMNS];
                        deserialize_row_to_strings (t1, rv, row_vals,
                                                    &local_arena);

                        char buffer[4096];
                        int b = 0;
                        b += snprintf (buffer + b, sizeof (buffer) - b, "(");

                        for (int c = 0; c < output_count; c++)
                        {
                            if (b >= sizeof (buffer) - 10)
                                break;
                            ColMap map = output_cols[c];
                            str8 val = row_vals[output_cols[c].idx];
                            bool is_text = output_cols[c]
                                               .t->columns[output_cols[c].idx]
                                               .type
                                           == TYPE_TEXT;

                            if (c > 0)
                                b += snprintf (buffer + b, sizeof (buffer) - b,
                                               ", ");
                            if (is_text)
                                b += snprintf (buffer + b, sizeof (buffer) - b,
                                               "\"%.*s\"", STR_FMT (val));
                            else
                                b += snprintf (buffer + b, sizeof (buffer) - b,
                                               "%.*s", STR_FMT (val));
                        }

                        if (b < sizeof (buffer) - 2)
                            b += snprintf (buffer + b, sizeof (buffer) - b,
                                           ")\n");
                        else
                        {
                            buffer[sizeof (buffer) - 2] = ')';
                            buffer[sizeof (buffer) - 1] = '\n';
                        }

                        if (send (client_fd, buffer, b, MSG_NOSIGNAL) == -1)
                        {
                            temp_arena_memory_end (print_scratch);
                            return EXECUTE_SUCCESS;
                        }
                        // Reset local arena
                        temp_arena_memory_end (print_scratch);
                    }
                }
            }
        }
    }
    else
    {
        Table *where_table = NULL;
        int where_col_idx = -1;
        void *where_val_ptr = NULL;
        int32_t where_val_int;
        str8 where_val_str;

        if (stmt->select.has_where)
        {
            if (resolve_column (t1, t2, stmt->select.where_column, &where_table,
                                &where_col_idx)
                == -1)
                return EXECUTE_COL_NOT_FOUND;
            if (where_table->columns[where_col_idx].type == TYPE_INT)
            {
                char temp[32];
                snprintf (temp, 32, "%.*s", STR_FMT (stmt->select.where_value));
                where_val_int = atoi (temp);
                where_val_ptr = &where_val_int;
            }
            else
            {
                where_val_str = stmt->select.where_value;
                where_val_ptr = &where_val_str;
            }
        }

        Table *join_l_t = NULL;
        int join_l_idx = -1;
        Table *join_r_t = NULL;
        int join_r_idx = -1;
        if (stmt->select.has_join)
        {
            if (resolve_column (t1, t2, stmt->select.left_join_col, &join_l_t,
                                &join_l_idx)
                == -1)
                return EXECUTE_COL_NOT_FOUND;
            if (resolve_column (t1, t2, stmt->select.right_join_col, &join_r_t,
                                &join_r_idx)
                == -1)
                return EXECUTE_COL_NOT_FOUND;
        }

        SlottedPageHeader *h1 = (SlottedPageHeader *) pager_get_page (
            db->global_arena, db->pager, t1->root_page_num);
        Slot *slots1 = (Slot *) ((uint8_t *) h1 + sizeof (SlottedPageHeader));
        SlottedPageHeader *h2 = NULL;
        Slot *slots2 = NULL;
        if (t2)
        {
            h2 = (SlottedPageHeader *) pager_get_page (
                db->global_arena, db->pager, t2->root_page_num);
            slots2 = (Slot *) ((uint8_t *) h2 + sizeof (SlottedPageHeader));
        }

        for (int i = 0; i < h1->num_cells; i++)
        {
            if (slots1[i].size == 0)
                continue;
            void *key1, *val1;
            uint32_t klen1, vlen1;
            slot_get_content (h1, i, &key1, &klen1, &val1, &vlen1);

            if (stmt->select.has_where && where_table == t1)
            {
                if (!row_matches_predicate (t1, val1, where_col_idx,
                                            where_val_ptr))
                    continue;
            }

            Temp_Arena_Memory outer_scratch =
                temp_arena_memory_begin (&local_arena);

            str8 t1_vals[MAX_COLUMNS];
            deserialize_row_to_strings (t1, val1, t1_vals, &local_arena);
            int loop_count = (t2) ? h2->num_cells : 1;

            for (int j = 0; j < loop_count; j++)
            {
                Temp_Arena_Memory inner_scratch =
                    temp_arena_memory_begin (&local_arena);
                str8 *t2_vals = NULL;

                if (t2)
                {
                    if (slots2[j].size == 0)
                    {
                        temp_arena_memory_end (inner_scratch);
                        continue;
                    }
                    void *key2, *val2;
                    uint32_t klen2, vlen2;
                    slot_get_content (h2, j, &key2, &klen2, &val2, &vlen2);

                    if (stmt->select.has_where && where_table == t2)
                    {
                        if (!row_matches_predicate (t2, val2, where_col_idx,
                                                    where_val_ptr))
                        {
                            temp_arena_memory_end (inner_scratch);
                            continue;
                        }
                    }
                    t2_vals = push_array_zero (&local_arena, str8, MAX_COLUMNS);
                    deserialize_row_to_strings (t2, val2, t2_vals,
                                                &local_arena);

                    if (stmt->select.has_join)
                    {
                        str8 val_l = (join_l_t == t1) ? t1_vals[join_l_idx]
                                                      : t2_vals[join_l_idx];
                        str8 val_r = (join_r_t == t1) ? t1_vals[join_r_idx]
                                                      : t2_vals[join_r_idx];
                        if (!str8_match (val_l, val_r, false))
                        {
                            temp_arena_memory_end (inner_scratch);
                            continue;
                        }
                    }
                }

                char buffer[4096];
                int b = 0;
                b += snprintf (buffer + b, sizeof (buffer) - b, "(");

                for (int k = 0; k < output_count; k++)
                {
                    if (b >= sizeof (buffer) - 10)
                        break;

                    ColMap map = output_cols[k];
                    str8 val =
                        (map.t == t1) ? t1_vals[map.idx] : t2_vals[map.idx];
                    bool is_text = map.t->columns[map.idx].type == TYPE_TEXT;

                    if (k > 0)
                        b += snprintf (buffer + b, sizeof (buffer) - b, ", ");
                    if (is_text)
                        b += snprintf (buffer + b, sizeof (buffer) - b,
                                       "\"%.*s\"", STR_FMT (val));
                    else
                        b += snprintf (buffer + b, sizeof (buffer) - b, "%.*s",
                                       STR_FMT (val));
                }

                if (b < sizeof (buffer) - 2)
                    b += snprintf (buffer + b, sizeof (buffer) - b, ")\n");
                else
                {
                    buffer[sizeof (buffer) - 2] = ')';
                    buffer[sizeof (buffer) - 1] = '\n';
                }

                if (send (client_fd, buffer, b, MSG_NOSIGNAL) == -1)
                {
                    temp_arena_memory_end (inner_scratch);
                    temp_arena_memory_end (outer_scratch);
                    return EXECUTE_SUCCESS;
                }

                temp_arena_memory_end (inner_scratch);
            }
            temp_arena_memory_end (outer_scratch);
        }
    }

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_delete (Statement *stmt, Database *db)
{
    unsigned char local_buffer[PAGE_SIZE];
    Arena local_arena;
    arena_init (&local_arena, local_buffer, sizeof (local_buffer));

    Table *t = db_find_table (db, stmt->delete.table_name);
    if (!t)
    {
        return EXECUTE_TABLE_NOT_EXISTS;
    }

    int target_col_idx = -1;
    if (stmt->delete.has_where)
    {
        for (int i = 0; i < t->col_count; i++)
        {
            if (str8_match (t->columns[i].name, stmt->delete.where_col.col_name,
                            true))
            {
                target_col_idx = i;
                break;
            }
        }

        if (target_col_idx == -1)
        {
            return EXECUTE_COL_NOT_FOUND;
        }
    }

    void *root_node =
        pager_get_page (db->global_arena, db->pager, t->root_page_num);
    SlottedPageHeader *header = (SlottedPageHeader *) root_node;
    Slot *slots = (Slot *) ((uint8_t *) root_node + sizeof (SlottedPageHeader));

    int32_t target_int_val;
    str8 target_str_val;
    void *target_ptr = NULL;

    if (stmt->delete.has_where)
    {
        ColumnDef *col = &t->columns[target_col_idx];
        if (col->type == TYPE_INT)
        {
            char temp[32];
            snprintf (temp, sizeof (temp), "%.*s",
                      STR_FMT (stmt->delete.where_value));
            target_int_val = atoi (temp);
            target_ptr = &target_int_val;
        }
        else
        {
            target_str_val = stmt->delete.where_value;
            target_ptr = &target_str_val;
        }
    }

    int delete_count = 0;
    int pk_idx = table_find_primary_key_index (t);
    if (pk_idx == -1)
    {
        pk_idx = 0;
    }

    for (int i = 0; i < header->num_cells; i++)
    {
        if (slots[i].size == 0)
        {
            continue;
        }

        void *key, *val;
        uint32_t key_len, val_len;
        slot_get_content (root_node, i, &key, &key_len, &val, &val_len);

        bool should_delete =
            !stmt->delete.has_where
            || row_matches_predicate (t, val, target_col_idx, target_ptr);
        ;

        if (should_delete)
        {
            Temp_Arena_Memory scratch = temp_arena_memory_begin (&local_arena);
            str8 row_vals[MAX_COLUMNS];
            deserialize_row_to_strings (t, val, row_vals, &local_arena);

            for (int idx_i = 0; idx_i < db->index_count; idx_i++)
            {
                Index *idx = &db->indexes[idx_i];
                if (str8_match (idx->table_name, t->table_name, true))
                {
                    int idx_col = table_find_col_index (t, idx->col_name);
                    if (idx_col == -1)
                    {
                        continue;
                    }

                    str8 val_to_remove = row_vals[idx_col];
                    str8 pk_val = row_vals[pk_idx];

                    void *idx_page = pager_get_page (
                        db->global_arena, db->pager, idx->root_page_num);
                    SlottedPageHeader *idx_h = (SlottedPageHeader *) idx_page;
                    Slot *idx_slots = (Slot *) ((uint8_t *) idx_page
                                                + sizeof (SlottedPageHeader));

                    for (int k = 0; k < idx_h->num_cells; k++)
                    {
                        if (idx_slots[k].size == 0)
                        {
                            continue;
                        }

                        void *ikey, *ival;
                        uint32_t iklen, ivlen;
                        slot_get_content (idx_page, k, &ikey, &iklen, &ival,
                                          &ivlen);

                        if (iklen == val_to_remove.len
                            && memcmp (ikey, val_to_remove.str, iklen) == 0
                            && ivlen == pk_val.len
                            && memcmp (ival, pk_val.str, ivlen) == 0)
                        {
                            idx_slots[k].size = 0;
                            pager_flush (db->pager, idx->root_page_num);
                            break;
                        }
                    }
                }
            }
            temp_arena_memory_end (scratch);

            slots[i].size = 0;
            slots[i].offset = 0;
            delete_count++;
        }
    }

    if (delete_count > 0)
    {
        pager_flush (db->pager, t->root_page_num);
    }

    return EXECUTE_SUCCESS;
}

static ExecuteResult execute_update (Statement *stmt, Database *db)
{
    unsigned char local_buffer[131072]; // 128 KB for pending updates
    Arena local_arena;
    arena_init (&local_arena, local_buffer, sizeof (local_buffer));

    Table *t = db_find_table (db, stmt->update.table_name);
    if (!t)
    {
        return EXECUTE_TABLE_NOT_EXISTS;
    }

    int where_col_idx = -1;
    void *target_ptr = NULL;
    int32_t target_int_val;
    str8 target_str_val;

    if (stmt->update.has_where)
    {
        for (int i = 0; i < t->col_count; i++)
        {
            if (str8_match (t->columns[i].name, stmt->update.where_col.col_name,
                            true))
            {
                where_col_idx = i;
                break;
            }
        }

        if (where_col_idx == -1)
            return EXECUTE_COL_NOT_FOUND;

        if (t->columns[where_col_idx].type == TYPE_INT)
        {
            char temp_buf[32];
            snprintf (temp_buf, 32, "%.*s", STR_FMT (stmt->update.where_value));
            target_int_val = atoi (temp_buf);
            target_ptr = &target_int_val;
        }
        else
        {
            target_str_val = stmt->update.where_value;
            target_ptr = &target_str_val;
        }
    }

    int assign_idxs[MAX_COLUMNS];
    for (int j = 0; j < stmt->update.assign_col_count; j++)
    {
        assign_idxs[j] = -1;
        for (int i = 0; i < t->col_count; i++)
        {
            if (str8_match (t->columns[i].name,
                            stmt->update.assignments[j].col_name, true))
            {
                assign_idxs[j] = i;
                break;
            }
        }
        if (assign_idxs[j] == -1)
        {
            return EXECUTE_COL_NOT_FOUND;
        }
    }

    void *root_node =
        pager_get_page (db->global_arena, db->pager, t->root_page_num);
    SlottedPageHeader *header = (SlottedPageHeader *) root_node;
    Slot *slots = (Slot *) ((uint8_t *) root_node + sizeof (SlottedPageHeader));

    typedef struct
    {
        uint8_t *data;
        uint32_t len;

        uint32_t key_val_int;
        void *key_ptr;
        uint32_t key_len;
    } PendingRows;

    PendingRows *pending_inserts =
        push_array_no_zero (&local_arena, PendingRows, 100);
    int pending_count = 0;
    int updated_count = 0;

    int pk_idx = table_find_primary_key_index (t);
    if (pk_idx == -1)
    {
        pk_idx = 0;
    }

    for (int i = 0; i < header->num_cells; i++)
    {
        if (slots[i].size == 0)
        {
            continue;
        }

        void *key, *val;
        uint32_t key_len, val_len;
        slot_get_content (root_node, i, &key, &key_len, &val, &val_len);

        bool match =
            !stmt->update.has_where
            || row_matches_predicate (t, val, where_col_idx, target_ptr);

        if (match)
        {
            Temp_Arena_Memory row_scratch =
                temp_arena_memory_begin (&local_arena);
            str8 row_values[MAX_COLUMNS];
            deserialize_row_to_strings (t, val, row_values, &local_arena);

            for (int idx_i = 0; idx_i < db->index_count; idx_i++)
            {
                Index *idx = &db->indexes[idx_i];
                if (!str8_match (idx->table_name, t->table_name, true))
                {
                    continue;
                }

                int idx_col = table_find_col_index (t, idx->col_name);

                int assign_entry = -1;
                for (int a = 0; a < stmt->update.assign_col_count; a++)
                {
                    if (assign_idxs[a] == idx_col)
                    {
                        assign_entry = a;
                        break;
                    }
                }

                if (assign_entry != -1)
                {
                    str8 old_val = row_values[idx_col];
                    str8 new_val = stmt->update.assignments[assign_entry].value;
                    str8 pk_val = row_values[pk_idx];

                    void *idx_page = pager_get_page (
                        db->global_arena, db->pager, idx->root_page_num);
                    SlottedPageHeader *idx_h = (SlottedPageHeader *) idx_page;
                    Slot *idx_slots = (Slot *) ((uint8_t *) idx_page
                                                + sizeof (SlottedPageHeader));

                    for (int k = 0; k < idx_h->num_cells; k++)
                    {
                        if (idx_slots[k].size == 0)
                        {
                            continue;
                        }
                        void *ik, *iv;
                        uint32_t ikl, ivl;
                        slot_get_content (idx_page, k, &ik, &ikl, &iv, &ivl);
                        if (ikl == old_val.len
                            && memcmp (ik, old_val.str, ikl) == 0
                            && ivl == pk_val.len
                            && memcmp (iv, pk_val.str, ivl) == 0)
                        {
                            idx_slots[k].size = 0; // Tombstone
                            break;
                        }
                    }
                    pager_slotted_insert (idx_page, new_val.str, new_val.len,
                                          pk_val.str, pk_val.len);
                    pager_flush (db->pager, idx->root_page_num);
                }
            }

            for (int j = 0; j < stmt->update.assign_col_count; j++)
            {
                int col_idx = assign_idxs[j];
                row_values[col_idx] = stmt->update.assignments[j].value;
            }

            uint8_t *new_row_buf =
                push_array_zero (&local_arena, uint8_t, PAGE_SIZE);
            if (!new_row_buf)
                return EXECUTE_DB_FULL; // buffer full

            uint32_t new_size = serialize_row (t, row_values, new_row_buf);

            uint32_t cell_header_size = sizeof (uint32_t) + key_len;
            uint32_t total_new_size = cell_header_size + new_size;

            if (total_new_size <= slots[i].size)
            {
                uint8_t *dest =
                    (uint8_t *) root_node + slots[i].offset + cell_header_size;
                memcpy (dest, new_row_buf, new_size);
                slots[i].size = total_new_size;

                temp_arena_memory_end (row_scratch);
            }
            else
            {
                slots[i].size = 0;
                if (pending_count < 100)
                {
                    pending_inserts[pending_count].data = new_row_buf;
                    pending_inserts[pending_count].len = new_size;
                    if (t->columns[pk_idx].type == TYPE_INT)
                    {
                        char temp[32];
                        snprintf (temp, 32, "%.*s",
                                  STR_FMT (row_values[pk_idx]));
                        pending_inserts[pending_count].key_val_int =
                            atoi (temp);
                        pending_inserts[pending_count].key_ptr =
                            &pending_inserts[pending_count].key_val_int;
                        pending_inserts[pending_count].key_len = 4;
                    }
                    else
                    {
                        pending_inserts[pending_count].key_ptr =
                            row_values[pk_idx].str;
                        pending_inserts[pending_count].key_len =
                            row_values[pk_idx].len;
                    }
                    pending_count++;
                }
                else
                {
                    temp_arena_memory_end (row_scratch);
                }
            }
            updated_count++;
        }
    }

    if (updated_count > 0)
    {
        pager_flush (db->pager, t->root_page_num);
    }

    for (int k = 0; k < pending_count; k++)
    {
        pager_slotted_insert (root_node, pending_inserts[k].key_ptr,
                              pending_inserts[k].key_len,
                              pending_inserts[k].data, pending_inserts[k].len);
    }

    if (pending_count > 0)
    {
        pager_flush (db->pager, t->root_page_num);
    }

    return EXECUTE_SUCCESS;
}

static bool row_matches_predicate (Table *t, void *row_data, int target_col_idx,
                                   void *target_val)
{
    uint8_t *ptr = (uint8_t *) row_data;

    for (int i = 0; i < t->col_count; i++)
    {
        ColumnDef *col = &t->columns[i];

        if (i == target_col_idx)
        {
            if (col->type == TYPE_INT)
            {
                int32_t row_int;
                memcpy (&row_int, ptr, sizeof (int32_t));

                int32_t target_int = *(int32_t *) target_val;
                return row_int == target_int;
            }
            else if (col->type == TYPE_TEXT)
            {
                uint32_t len;
                memcpy (&len, ptr, sizeof (uint32_t));
                ptr += sizeof (uint32_t);

                str8 *target_str = (str8 *) target_val;
                if (len != target_str->len)
                {
                    return false;
                }

                return memcmp (ptr, target_str->str, len) == 0;
            }
        }

        if (col->type == TYPE_INT)
        {
            ptr += sizeof (int32_t);
        }
        else if (col->type == TYPE_TEXT)
        {
            uint32_t len;
            memcpy (&len, ptr, sizeof (uint32_t));
            ptr += sizeof (uint32_t) + len;
        }
    }

    return false;
}
