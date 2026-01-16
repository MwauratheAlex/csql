#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"
#include "../str/str.h"
#include "../token/token.h"
#include "stdbool.h"

#define MAX_COLUMNS 16

typedef enum
{
    STMT_SELECT,
    STMT_INSERT,
    STMT_CREATE_TABLE,
    STMT_CREATE_INDEX,
    STMT_UPDATE,
    STMT_DELETE,

    STMT_ERROR,
} StatementType;

typedef enum
{
    TYPE_INT,
    TYPE_TEXT,
} DataType;

// CREATE TABLE users (id int, name text);
typedef struct
{
    str8 name;
    DataType type;

    bool is_primary_key;
    bool is_unique;
} ColumnDef;

typedef struct
{
    str8 table_name;
    int col_count;
    ColumnDef columns[MAX_COLUMNS];
} CreateStmt;

typedef struct
{
    str8 index_name;
    str8 table_name;
    str8 col_name;
} CreateIndexStmt;

// INSERT INTO users VALUES (1, 'john');
typedef struct
{
    str8 table_name;
    int val_count;
    str8 values[MAX_COLUMNS];
} InsertStmt;

// SELECT * FROM users WHERE id = 1;
typedef struct
{
    str8 table_name;
    str8 col_name;
} ColumnRef;

typedef struct
{
    str8 table_name;
    int field_count;
    ColumnRef fields[MAX_COLUMNS];

    bool has_join;
    str8 join_table;

    ColumnRef left_join_col;
    ColumnRef right_join_col;

    bool has_where;
    ColumnRef where_column;
    str8 where_value;
} SelectStmt;

// Update
typedef struct
{
    str8 col_name;
    str8 value;
} AssignCol;

typedef struct
{
    str8 table_name;

    int assign_col_count;
    AssignCol assignments[MAX_COLUMNS];

    bool has_where;
    ColumnRef where_col;
    str8 where_value;
} UpdateStmt;

// Delete
typedef struct
{
    str8 table_name;

    bool has_where;
    ColumnRef where_col;
    str8 where_value;
} DeleteStmt;

// Error
typedef struct
{
    const char *msg;
} ErrorStmt;

typedef struct
{
    StatementType type;
    union
    {
        CreateStmt create;
        CreateIndexStmt create_index;
        InsertStmt insert;
        SelectStmt select;
        UpdateStmt update;
        DeleteStmt delete;
        ErrorStmt error;
    };
} Statement;

typedef struct
{
    Lexer l;
    Token curr;
    Token peek;
} Parser;

void parser_init (Parser *p, const char *input);
Statement parser_parse_statement (Parser *p);

const char *data_type_to_string (DataType type);

#endif /* PARSER_H */
