#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>

static void parser_next_token (Parser *p);
static Statement parser_parse_create_table (Parser *p);
static Statement parser_parse_create_index (Parser *p);
static Statement parser_parse_insert (Parser *p);
static Statement parser_parse_select (Parser *p);
static Statement parser_parse_delete (Parser *p);
static Statement parser_parse_update (Parser *p);
static Statement stmt_error (const char *msg);
static bool parser_expect (Parser *p, TokenType type);
static ColumnRef parser_parse_column_ref (Parser *p);

void parser_init (Parser *p, const char *input)
{
    lexer_init (&p->l, input);
    parser_next_token (p); // fill curr
    parser_next_token (p); // fill peek
}

static void parser_next_token (Parser *p)
{
    p->curr = p->peek;
    p->peek = lexer_next_token (&p->l);
}

static Statement stmt_error (const char *msg)
{
    Statement s;
    s.type = STMT_ERROR;
    s.error.msg = msg;
    return s;
}

Statement parser_parse_statement (Parser *p)
{
    switch (p->curr.type)
    {
    case TOKEN_CREATE:
        if (p->peek.type == TOKEN_INDEX)
        {
            return parser_parse_create_index (p);
        }
        return parser_parse_create_table (p);
    case TOKEN_INSERT:
        return parser_parse_insert (p);
    case TOKEN_SELECT:
        return parser_parse_select (p);
    case TOKEN_UPDATE:
        return parser_parse_update (p);
    case TOKEN_DELETE:
        return parser_parse_delete (p);
    default:
        return stmt_error ("Unexpected token");
    }
}

// Syntax: CREATE TABLE <name> ( <col> <type>, ... );
static Statement parser_parse_create_table (Parser *p)
{
    Statement s;
    s.type = STMT_CREATE_TABLE;
    s.create.col_count = 0;

    parser_next_token (p); // skip CREATE
    if (!parser_expect (p, TOKEN_TABLE))
    {
        return stmt_error ("Expected 'TABLE' after CREATE");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected table name");
    }

    s.create.table_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_LPAREN))
    {
        return stmt_error ("Expected '(' after table name");
    }

    // parse columns
    bool first = true;
    while (p->curr.type != TOKEN_RPAREN && p->curr.type != TOKEN_EOF)
    {
        if (!first && !parser_expect (p, TOKEN_COMMA))
        {
            break;
        }

        first = false;

        if (p->curr.type == TOKEN_RPAREN)
        {
            return stmt_error ("Trailing comma or unexpected end");
        }

        if (s.create.col_count >= MAX_COLUMNS)
        {
            return stmt_error ("Too many columns");
        }

        if (p->curr.type != TOKEN_IDENT)
        {
            return stmt_error ("Expected column name");
        }

        str8 col_name = p->curr.literal;
        parser_next_token (p);

        DataType column_type;
        if (p->curr.type == TOKEN_INT_TYPE)
        {
            column_type = TYPE_INT;
        }
        else if (p->curr.type == TOKEN_TEXT_TYPE)
        {
            column_type = TYPE_TEXT;
        }
        else
        {
            return stmt_error ("Expected type 'int' or type 'text'");
        }

        parser_next_token (p);

        bool is_pk = false;
        bool is_unique = false;

        if (p->curr.type == TOKEN_PRIMARY)
        {
            parser_next_token (p);
            if (!parser_expect (p, TOKEN_KEY))
            {
                return stmt_error ("Expected 'KEY' after 'PRIMARY'");
            }
            is_pk = true;
        }

        if (p->curr.type == TOKEN_UNIQUE)
        {
            parser_next_token (p);
            is_unique = true;
        }

        int i = s.create.col_count++;
        s.create.columns[i].name = col_name;
        s.create.columns[i].type = column_type;
        s.create.columns[i].is_primary_key = is_pk;
        s.create.columns[i].is_unique = is_unique;
    }

    if (!parser_expect (p, TOKEN_RPAREN))
    {
        return stmt_error ("Expected ')' after columns");
    }

    if (!parser_expect (p, TOKEN_SEMICOLON))
    {
        return stmt_error ("Expected ';' at end");
    }

    return s;
}

static Statement parser_parse_create_index (Parser *p)
{
    Statement s;
    s.type = STMT_CREATE_INDEX;

    parser_next_token (p); // skip create
    if (!parser_expect (p, TOKEN_INDEX))
    {
        return stmt_error ("Expected 'INDEX'");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected index name");
    }

    s.create_index.index_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_ON))
    {
        return stmt_error ("Expected 'ON' after index name");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected table name");
    }
    s.create_index.table_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_LPAREN))
    {
        return stmt_error ("Expected '('");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected column name");
    }
    s.create_index.col_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_RPAREN))
    {
        return stmt_error ("Expected ')'");
    }

    return s;
}

// Syntax: INSERT INTO <table_name> VALUES ( <value>, ... );
static Statement parser_parse_insert (Parser *p)
{
    Statement s;
    s.type = STMT_INSERT;
    s.insert.val_count = 0;

    parser_next_token (p); // skip INSERT

    if (!parser_expect (p, TOKEN_INTO))
    {
        return stmt_error ("Expected 'INTO' after INSERT");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected table name after INTO");
    }

    s.insert.table_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_VALUES))
    {
        return stmt_error ("Expected 'VALUES' after table name");
    }

    if (!parser_expect (p, TOKEN_LPAREN))
    {
        return stmt_error ("Expected '(' after VALUES");
    }

    bool first = true;
    while (p->curr.type != TOKEN_RPAREN && p->curr.type != TOKEN_EOF)
    {
        if (!first && !parser_expect (p, TOKEN_COMMA))
        {
            break;
        }

        first = false;

        if (p->curr.type == TOKEN_RPAREN)
        {
            return stmt_error (
                "Trailing comma or unexpected end in VALUE list");
        }

        if (s.insert.val_count >= MAX_COLUMNS)
        {
            return stmt_error ("Too many values");
        }

        if (p->curr.type == TOKEN_INT || p->curr.type == TOKEN_STRING)
        {
            s.insert.values[s.insert.val_count++] = p->curr.literal;
            parser_next_token (p);
        }
        else
        {
            return stmt_error ("Expected integer or string literal");
        }
    }

    if (!parser_expect (p, TOKEN_RPAREN))
    {
        return stmt_error ("Expected ')' after values");
    }

    if (!parser_expect (p, TOKEN_SEMICOLON))
    {
        return stmt_error ("Expected ';' at end");
    }

    return s;
}

// Syntax: SELECT <* | col, ...> FROM <table_name> [JOIN <table_name> ON
// <col> = <col>] [WHERE <col> = <val>];
static Statement parser_parse_select (Parser *p)
{
    Statement s;
    s.type = STMT_SELECT;
    s.select.field_count = 0;
    s.select.has_join = false;
    s.select.has_where = false;

    parser_next_token (p);

    if (p->curr.type == TOKEN_ASTERISK)
    {
        parser_next_token (p);
    }
    else
    {
        bool first = true;
        while (p->curr.type != TOKEN_FROM && p->curr.type != TOKEN_EOF)
        {
            if (!first && !parser_expect (p, TOKEN_COMMA))
            {
                break;
            }
            first = false;

            ColumnRef col = parser_parse_column_ref (p);
            if (col.col_name.str == NULL)
            {
                return stmt_error ("Expected column name or '*' in SELECT");
            }

            if (s.select.field_count >= MAX_COLUMNS)
            {
                return stmt_error ("Too many columns in SELECT");
            }

            s.select.fields[s.select.field_count++] = col;
        }
    }

    if (!parser_expect (p, TOKEN_FROM))
    {
        return stmt_error ("Expected 'FROM' after field list");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected table name");
    }
    s.select.table_name = p->curr.literal;
    parser_next_token (p);

    if (p->curr.type == TOKEN_JOIN)
    {
        s.select.has_join = true;
        parser_next_token (p);

        if (p->curr.type != TOKEN_IDENT)
        {
            return stmt_error ("Expected table name after JOIN");
        }
        s.select.join_table = p->curr.literal;
        parser_next_token (p);

        if (!parser_expect (p, TOKEN_ON))
        {
            return stmt_error ("Expected 'ON' after JOIN table");
        }

        s.select.left_join_col = parser_parse_column_ref (p);
        if (s.select.left_join_col.col_name.str == NULL)
        {
            return stmt_error ("Expected left join column in ON");
        }

        if (!parser_expect (p, TOKEN_ASSIGN))
        {
            return stmt_error ("Expected '=' in ON");
        }

        s.select.right_join_col = parser_parse_column_ref (p);
        if (s.select.right_join_col.col_name.str == NULL)
        {
            return stmt_error ("Expected right join column in ON");
        }
    }

    if (p->curr.type == TOKEN_WHERE)
    {
        s.select.has_where = true;
        parser_next_token (p);

        s.select.where_column = parser_parse_column_ref (p);
        if (s.select.where_column.col_name.str == NULL)
        {
            return stmt_error ("Expected column in WHERE");
        }

        if (!parser_expect (p, TOKEN_ASSIGN))
        {
            return stmt_error ("Expected '=' in WHERE");
        }

        if (p->curr.type == TOKEN_INT || p->curr.type == TOKEN_STRING)
        {
            s.select.where_value = p->curr.literal;
            parser_next_token (p);
        }
        else
        {
            return stmt_error ("Expected value in WHERE");
        }
    }

    if (!parser_expect (p, TOKEN_SEMICOLON))
    {
        return stmt_error ("Expected ';' at end of SELECT");
    }

    return s;
}

static ColumnRef parser_parse_column_ref (Parser *p)
{
    ColumnRef ref = {0};
    if (p->curr.type != TOKEN_IDENT)
    {
        return ref;
    }

    str8 first_part = p->curr.literal;
    parser_next_token (p);

    if (p->curr.type == TOKEN_DOT)
    {
        parser_next_token (p);

        if (p->curr.type == TOKEN_IDENT)
        {
            ref.table_name = first_part;
            ref.col_name = p->curr.literal;
            parser_next_token (p);
        }
    }
    else
    {
        ref.col_name = first_part;
    }

    return ref;
}

// Syntax: DELETE FROM <table_name> [WHERE <col> = <val>];
static Statement parser_parse_delete (Parser *p)
{
    Statement s;
    s.type = STMT_DELETE;
    s.delete.has_where = false;

    parser_next_token (p);
    if (!parser_expect (p, TOKEN_FROM))
    {
        return stmt_error ("Expected 'FROM' after DELETE");
    }

    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected 'table name'");
    }

    s.delete.table_name = p->curr.literal;
    parser_next_token (p);

    if (p->curr.type == TOKEN_WHERE)
    {
        s.delete.has_where = true;
        parser_next_token (p);

        s.delete.where_col = parser_parse_column_ref (p);
        if (s.delete.where_col.col_name.str == NULL)
        {
            return stmt_error ("Expected column in WHERE");
        }

        if (!parser_expect (p, TOKEN_ASSIGN))
        {
            return stmt_error ("Expected '=' in WHERE");
        }
        if (p->curr.type == TOKEN_INT || p->curr.type == TOKEN_STRING)
        {
            s.delete.where_value = p->curr.literal;
            parser_next_token (p);
        }
        else
        {
            return stmt_error ("Expected value in WHERE");
        }
    }

    if (!parser_expect (p, TOKEN_SEMICOLON))
    {
        return stmt_error ("Expected ';'");
    }

    return s;
}

// Syntax: UPDATE <table_name> SET <col> = <val>, ... [WHERE <col> = <val>];
static Statement parser_parse_update (Parser *p)
{
    Statement s;
    s.type = STMT_UPDATE;
    s.update.assign_col_count = 0;
    s.update.has_where = false;

    parser_next_token (p);
    if (p->curr.type != TOKEN_IDENT)
    {
        return stmt_error ("Expected table name");
    }
    s.update.table_name = p->curr.literal;
    parser_next_token (p);

    if (!parser_expect (p, TOKEN_SET))
    {
        return stmt_error ("Expected SET");
    }

    bool first = true;
    while (p->curr.type != TOKEN_WHERE && p->curr.type != TOKEN_SEMICOLON
           && p->curr.type != TOKEN_EOF)
    {
        if (!first && !parser_expect (p, TOKEN_COMMA))
        {
            break;
        }
        first = false;

        if (p->curr.type != TOKEN_IDENT)
        {
            return stmt_error ("Expected column name");
        }
        str8 col_name = p->curr.literal;
        parser_next_token (p);

        if (!parser_expect (p, TOKEN_ASSIGN))
        {
            return stmt_error ("Expected '='");
        }

        if (p->curr.type != TOKEN_INT && p->curr.type != TOKEN_STRING)
        {
            return stmt_error ("Expected value (int or text) in SET clause");
        }
        str8 val = p->curr.literal;
        parser_next_token (p);

        if (s.update.assign_col_count >= MAX_COLUMNS)
        {
            return stmt_error ("Too many assignments");
        }

        int i = s.update.assign_col_count++;
        s.update.assignments[i].col_name = col_name;
        s.update.assignments[i].value = val;
    }

    if (p->curr.type == TOKEN_WHERE)
    {
        s.update.has_where = true;
        parser_next_token (p);

        s.update.where_col = parser_parse_column_ref (p);
        if (s.update.where_col.col_name.str == NULL)
        {
            return stmt_error ("Expected column in WHERE");
        }
        if (!parser_expect (p, TOKEN_ASSIGN))
        {
            return stmt_error ("Expected '='");
        }

        if (p->curr.type == TOKEN_INT || p->curr.type == TOKEN_STRING)
        {
            s.update.where_value = p->curr.literal;
            parser_next_token (p);
        }
        else
        {
            return stmt_error ("Expected value in WHERE");
        }
    }

    if (!parser_expect (p, TOKEN_SEMICOLON))
    {
        return stmt_error ("Expected ';' at end");
    }

    return s;
}

static bool parser_expect (Parser *p, TokenType type)
{
    if (p->curr.type != type)
    {
        return false;
    }
    parser_next_token (p);
    return true;
}

const char *data_type_to_string (DataType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "INT";
    case TYPE_TEXT:
        return "TEXT";
    default:
        return "UNKNOWN";
    }
}
