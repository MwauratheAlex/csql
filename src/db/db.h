#ifndef DB_H
#define DB_H

#include "../pager/pager.h"
#include "../parser/parser.h"
#include "../str/str.h"

#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdint.h>

#define MAX_TABLES     100
#define MAX_INDEXES    20
#define MAX_TABLE_NAME 32

typedef struct
{
    str8 table_name;
    uint32_t root_page_num;
    uint32_t col_count;
    ColumnDef columns[MAX_COLUMNS];
    Pager *pager;
} Table;

typedef struct
{
    str8 index_name;
    str8 table_name;
    str8 col_name;
    uint32_t root_page_num;
} Index;

typedef struct
{
    Pager *pager;
    Table *tables[MAX_TABLES];
    int table_count;
    pthread_mutex_t lock;

    int index_count;
    Index indexes[MAX_INDEXES];

    Arena *global_arena;
} Database;

Table *db_find_table (Database *db, str8 name);
void db_create_table (str8 name);

void catalog_init_from_disk (Database *db);
uint32_t serialize_table (Table *table, void *dest);
void deserialize_table (Arena *arena, void *val, Table *t);
uint32_t serialize_row (Table *table, str8 *values, void *dest);
void deserialize_print_row (Table *table, void *row_data, int client_fd);
void deserialize_row_to_strings (Table *t, void *row_data, str8 *out_values,
                                 Arena *arena);
int table_find_col_index (Table *t, str8 col_name);
int resolve_column (Table *t1, Table *t2, ColumnRef ref, Table **out_table,
                    int *out_col_idx);
int table_find_primary_key_index (Table *t);

#endif /* DB_H */
