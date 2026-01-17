#include "db.h"

#include "../pager/pager.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * load_catalog - loads the catalog to the database
 * @db: database pointer
 *
 * -- Catalog Structure on Disk
 *  table_count(4bytes) |
 *  table(MAX_TABLE_SIZE(32bytes) + table_page_num(4bytes) = 36bytes), ...
 *
 * Return: nothing
 * */
void catalog_init_from_disk (Database *db)
{
    void *page_zero = pager_get_page (db->global_arena, db->pager, 0);
    SlottedPageHeader *header = (SlottedPageHeader *) page_zero;

    db->table_count = 0;
    for (int i = 0; i < header->num_cells; i++)
    {
        void *key;
        uint32_t key_len;
        void *val;
        uint32_t val_len;

        slot_get_content (page_zero, i, &key, &key_len, &val, &val_len);

        Table *t = push_struct_zero (db->global_arena, Table);
        t->table_name.str =
            push_array_no_zero (db->global_arena, uint8_t, key_len + 1);
        memcpy (t->table_name.str, key, key_len);
        t->table_name.str[key_len] = '\0';
        t->table_name.len = key_len;
        deserialize_table (db->global_arena, val, t);

        t->pager = db->pager;

        if (db->table_count < MAX_TABLES)
        {
            db->tables[db->table_count++] = t;
            printf ("Loaded Table: %.*s (Columns: %d)\n",
                    (int) t->table_name.len, t->table_name.str, t->col_count);
        }
        else
        {
            printf ("Warning: Catalog contains more tables than memory cache "
                    "can hold.\n");
        }
    }
}

/**
 * db_find_table - finds a table in the catalog by name
 * @db: pointer to the database struct
 * @name: the table name to search for (as str8)
 *
 * Return: pointer to the Table struct if found, NULL otherwise
 */
Table *db_find_table (Database *db, str8 name)
{
    for (int i = 0; i < db->table_count; i++)
    {
        Table *t = db->tables[i];

        if (str8_match (t->table_name, name, false))
        {
            return t;
        }
    }

    return NULL;
}

/**
 * serialize_table - serializes a table and writes it to dest
 * @table: table pointer
 * @dest: destination pointer
 * Returns: offset
 */
uint32_t serialize_table (Table *table, void *dest)
{
    uint8_t *d = (uint8_t *) dest;
    uint32_t offset = 0;

    // root page
    memcpy (d + offset, &table->root_page_num, sizeof (uint32_t));
    offset += sizeof (uint32_t);

    // column count
    memcpy (d + offset, &table->col_count, sizeof (uint32_t));
    offset += sizeof (uint32_t);

    // Columns
    for (int i = 0; i < table->col_count; i++)
    {
        // Type
        ColumnDef *col = &table->columns[i];
        memcpy (d + offset, &col->type, sizeof (DataType));
        offset += sizeof (DataType);

        // Name Length
        uint32_t name_len = col->name.len;
        memcpy (d + offset, &name_len, sizeof (uint32_t));
        offset += sizeof (uint32_t);

        // Name
        memcpy (d + offset, col->name.str, name_len);
        offset += name_len;

        // Flags
        memcpy (d + offset, &col->is_primary_key, sizeof (bool));
        offset += sizeof (bool);
        memcpy (d + offset, &col->is_unique, sizeof (bool));
        offset += sizeof (bool);
    }

    return offset;
}

/**
 * deserialize_table - serializes a table and writes it to dest
 * @table: table pointer
 * @dest: destination pointer
 * Returns: offset
 */
void deserialize_table (Arena *arena, void *val, Table *table)
{
    uint8_t *src = (uint8_t *) val;
    uint32_t offset = 0;

    // Root page
    memcpy (&table->root_page_num, src + offset, sizeof (uint32_t));
    offset += sizeof (uint32_t);

    // Column count
    memcpy (&table->col_count, src + offset, sizeof (uint32_t));
    offset += sizeof (uint32_t);

    // Columns
    for (int i = 0; i < table->col_count; i++)
    {
        ColumnDef *col = &table->columns[i];

        // type
        memcpy (&col->type, src + offset, sizeof (DataType));
        offset += sizeof (DataType);

        // name len
        uint32_t name_len;
        memcpy (&name_len, src + offset, sizeof (uint32_t));
        offset += sizeof (uint32_t);
        col->name.len = name_len;

        // name
        col->name.str = push_array_no_zero (arena, uint8_t, name_len + 1);
        memcpy (col->name.str, src + offset, name_len);
        col->name.str[name_len] = '\0';
        offset += name_len;

        // Flags
        memcpy (&col->is_primary_key, src + offset, sizeof (bool));
        offset += sizeof (bool);
        memcpy (&col->is_unique, src + offset, sizeof (bool));
        offset += sizeof (bool);
    }
}

/**
 * serialize_row - serializes a table row and writes it to dest
 * @table: table pointer
 * @values: array of table values
 * @dest: destination pointer
 * Returns: offset
 */
uint32_t serialize_row (Table *table, str8 *values, void *dest)
{
    uint8_t *d = (uint8_t *) dest;
    uint8_t *start = d;
    uint32_t offset = 0;

    for (int i = 0; i < table->col_count; i++)
    {
        ColumnDef *col = &table->columns[i];

        if (col->type == TYPE_INT)
        {
            int32_t val = atoi ((char *) values[i].str);
            memcpy (d + offset, &val, sizeof (uint32_t));
            offset += sizeof (int32_t);
        }
        else if (col->type == TYPE_TEXT)
        {
            uint32_t len = values[i].len;
            memcpy (d + offset, &len, sizeof (uint32_t));
            offset += sizeof (int32_t);

            memcpy (d + offset, values[i].str, len);
            offset += len;
        }
    }
    return offset;
}

void deserialize_print_row (Table *table, void *row_data, int client_fd)
{
    uint8_t *ptr = (uint8_t *) row_data;
    uint32_t offset = 0;

    char buffer[4096];
    int buf_len = 0;

    buf_len += snprintf (buffer + buf_len, sizeof (buffer) - buf_len, "(");

    for (int i = 0; i < table->col_count; i++)
    {
        ColumnDef *col = &table->columns[i];
        if (col->type == TYPE_INT)
        {
            int32_t val;
            memcpy (&val, ptr + offset, sizeof (int32_t));
            offset += sizeof (int32_t);
            buf_len += snprintf (buffer + buf_len, sizeof (buffer) - buf_len,
                                 "%d", val);
        }
        else if (col->type == TYPE_TEXT)
        {
            uint32_t len;
            memcpy (&len, ptr + offset, sizeof (uint32_t));
            offset += sizeof (uint32_t);

            buf_len += snprintf (buffer + buf_len, sizeof (buffer) - buf_len,
                                 "\"%.*s\"", len, ptr + offset);
            offset += len;
        }

        if (i < table->col_count - 1)
        {
            buf_len +=
                snprintf (buffer + buf_len, sizeof (buffer) - buf_len, ", ");
        }
    }

    buf_len += snprintf (buffer + buf_len, sizeof (buffer) - buf_len, ")\n");
    send (client_fd, buffer, buf_len, MSG_NOSIGNAL);
}

void deserialize_row_to_strings (Table *t, void *row_data, str8 *out_values,
                                 Arena *arena)
{
    uint8_t *ptr = (uint8_t *) row_data;

    for (int i = 0; i < t->col_count; i++)
    {
        ColumnDef *col = &t->columns[i];
        if (col->type == TYPE_INT)
        {
            int32_t val;
            memcpy (&val, ptr, sizeof (int32_t));
            ptr += sizeof (int32_t);

            char *buf = push_array_no_zero (arena, char, 32);
            int len = snprintf (buf, 32, "%d", val);
            out_values[i] =
                str8_from_range ((uint8_t *) buf, (uint8_t *) buf + len);
        }
        else if (col->type == TYPE_TEXT)
        {
            uint32_t len;
            memcpy (&len, ptr, sizeof (uint32_t));
            ptr += sizeof (uint32_t);

            uint8_t *str_copy = push_array_no_zero (arena, uint8_t, len);
            memcpy (str_copy, ptr, len);
            out_values[i] = str8_from_range (str_copy, str_copy + len);

            ptr += len;
        }
    }
}

int table_find_col_index (Table *t, str8 col_name)
{
    for (int i = 0; i < t->col_count; i++)
    {
        if (str8_match (t->columns[i].name, col_name, true))
        {
            return i;
        }
    }
    return -1;
}

int resolve_column (Table *t1, Table *t2, ColumnRef ref, Table **out_table,
                    int *out_col_idx)
{
    if (ref.table_name.len > 0)
    {
        if (str8_match (t1->table_name, ref.table_name, true))
        {
            *out_table = t1;
            *out_col_idx = table_find_col_index (t1, ref.col_name);
            return (*out_col_idx == -1) ? -1 : 0;
        }
        if (t2 && str8_match (t2->table_name, ref.table_name, true))
        {
            *out_table = t2;
            *out_col_idx = table_find_col_index (t2, ref.col_name);
            return (*out_col_idx == -1) ? -1 : 0;
        }
        return -1;
    }

    int idx = table_find_col_index (t1, ref.col_name);
    if (idx != -1)
    {
        *out_table = t1;
        *out_col_idx = idx;
        return 0;
    }

    if (t2)
    {
        idx = table_find_col_index (t2, ref.col_name);
        if (idx != -1)
        {
            *out_table = t2;
            *out_col_idx = idx;
            return 0;
        }
    }
    return -1;
}

int table_find_primary_key_index (Table *t)
{
    for (int i = 0; i < t->col_count; i++)
    {
        if (t->columns[i].is_primary_key)
        {
            return i;
        }
    }

    return -1;
}
