#include "../testing/testing.h"
#include "parser.h"

// unity includes
#include "../arena/arena.c"
#include "../lexer/lexer.c"
#include "../str/str.c"
#include "../token/token.c"
#include "parser.c"

#include <stdio.h>

void test_create_stmt ()
{
    char *input = "CREATE TABLE users (id int PRIMARY KEY, email text UNIQUE);";

    Parser p;
    parser_init (&p, input);

    Statement s = parser_parse_statement (&p);

    ASSERT_FMT (s.type == STMT_CREATE_TABLE,
                "tests[create] - statement type wrong. expected=CREATE. msg=%s",
                s.error.msg);

    ASSERT_FMT (str8_equals (s.create.table_name, str8_lit ("users")),
                "tests[create] - table name wrong. expected=%s, got=%.*s",
                "users", STR_FMT (s.create.table_name));

    ASSERT_FMT (s.create.col_count == 2,
                "tests[create] - column count wrong. expected=%d, got=%d", 2,
                s.create.col_count);

    ColumnDef expected_columns[3] = {
        {str8_lit ("id"), TYPE_INT, true, false},
        {str8_lit ("email"), TYPE_TEXT, false, true},
    };

    for (int i = 0; i < 2; i++)
    {
        ASSERT_FMT (
            str8_equals (s.create.columns[i].name, expected_columns[i].name),
            "tests[create] - column name wrong. expected=%.*s, got=%.*s",
            STR_FMT (expected_columns[i].name),
            STR_FMT (s.create.columns[i].name));

        ASSERT_FMT (s.create.columns[i].type == expected_columns[i].type,
                    "tests[create] - column type wrong. expected=%s, got=%s",
                    data_type_to_string (expected_columns[i].type),
                    data_type_to_string (s.create.columns[i].type));

        ASSERT_FMT (s.create.columns[i].is_primary_key
                        == expected_columns[i].is_primary_key,
                    "tests[create] - is primary key wrong. expected=%s, got=%s",
                    expected_columns[i].is_primary_key ? "true" : "false",
                    s.create.columns[i].is_primary_key ? "true" : "false");

        ASSERT_FMT (s.create.columns[i].is_unique
                        == expected_columns[i].is_unique,
                    "tests[create] - is primary key wrong. expected=%s, got=%s",
                    expected_columns[i].is_unique ? "true" : "false",
                    s.create.columns[i].is_unique ? "true" : "false");
    }
    printf ("PARSER: [create] All tests passed!\n");
}

void test_insert_stmt ()
{
    char *input = "INSERT INTO users VALUES (1, 'John Doe', 30);";

    Parser p;
    parser_init (&p, input);

    Statement s = parser_parse_statement (&p);

    ASSERT_FMT (s.type == STMT_INSERT,
                "tests[insert] - statement type wrong. expected=STMT_INSERT, "
                "got=%s (Error: %s)",
                s.type == STMT_ERROR ? "STMT_ERROR" : "OTHER",
                s.type == STMT_ERROR ? s.error.msg : "none");

    ASSERT_FMT (str8_equals (s.insert.table_name, str8_lit ("users")),
                "tests[insert] - table name wrong. expected=%s, got=%.*s",
                "users", STR_FMT (s.insert.table_name));

    ASSERT_FMT (s.insert.val_count == 3,
                "tests[insert] - value count wrong. expected=%d, got=%d", 3,
                s.insert.val_count);

    str8 expected_values[3] = {
        str8_lit ("1"),
        str8_lit ("John Doe"),
        str8_lit ("30"),
    };

    for (int i = 0; i < 3; i++)
    {
        ASSERT_FMT (
            str8_equals (s.insert.values[i], expected_values[i]),
            "tests[insert] - value at index %d wrong. expected=%.*s, got=%.*s",
            i, STR_FMT (expected_values[i]), STR_FMT (s.insert.values[i]));
    }

    printf ("PARSER: [insert] All tests passed!\n");
}

void test_select_stmt ()
{
    char *input = "SELECT users.name, posts.title FROM users JOIN posts ON "
                  "users.id = posts.user_id WHERE users.id = 1;";
    Parser p;
    parser_init (&p, input);

    Statement s = parser_parse_statement (&p);

    ASSERT_FMT (s.type == STMT_SELECT, "test[select] - Type should be SELECT");

    ASSERT_FMT (str8_equals (s.select.fields[0].table_name, str8_lit ("users")),
                "tests[select] - Field 0 table");
    ASSERT_FMT (str8_equals (s.select.fields[0].col_name, str8_lit ("name")),
                "test[select] - Field 0 name");

    ASSERT_FMT (s.select.has_join == true, "test[select] - Should have JOIN");
    ASSERT_FMT (str8_equals (s.select.join_table, str8_lit ("posts")),
                "test[select] - Join table name");

    ASSERT_FMT (
        str8_equals (s.select.left_join_col.table_name, str8_lit ("users")),
        "test[select] - Left Join table");
    ASSERT_FMT (str8_equals (s.select.left_join_col.col_name, str8_lit ("id")),
                "test[select] - Left Join col");

    ASSERT_FMT (
        str8_equals (s.select.right_join_col.table_name, str8_lit ("posts")),
        "test[select] - Right Join table");
    ASSERT_FMT (
        str8_equals (s.select.right_join_col.col_name, str8_lit ("user_id")),
        "test[select] - Right Join col");

    printf ("PARSER: [select+join] All tests passed!\n");
}

void test_delete_stmt ()
{
    char *input = "DELETE FROM users WHERE id = 5;";

    Parser p;
    parser_init (&p, input);
    Statement s = parser_parse_statement (&p);

    ASSERT_FMT (s.type == STMT_DELETE,
                "test[delete] - Type should be STMT_DELETE");

    ASSERT_FMT (str8_equals (s.delete.table_name, str8_lit ("users")),
                "test[delete] - Table name wrong. Expected=users, Got=%.*s",
                STR_FMT (s.delete.table_name));

    ASSERT_FMT (s.delete.has_where == true,
                "test[delete] - Should have WHERE clause");

    ASSERT_FMT (str8_equals (s.delete.where_col.col_name, str8_lit ("id")),
                "test[delete] - Where column wrong. Expected=id, Got=%.*s",
                STR_FMT (s.delete.where_col.col_name));

    ASSERT_FMT (str8_equals (s.delete.where_value, str8_lit ("5")),
                "test[delete] - Where value wrong. Expected=5, Got=%.*s",
                STR_FMT (s.delete.where_value));

    printf ("PARSER: [delete] All tests passed!\n");
}

void test_update_stmt ()
{
    char *input =
        "UPDATE users SET name = 'Jane', age = 30 WHERE users.id = 1;";

    Parser p;
    parser_init (&p, input);
    Statement s = parser_parse_statement (&p);

    ASSERT_FMT (s.type == STMT_UPDATE,
                "test[update] - Type should be STMT_UPDATE");

    ASSERT_FMT (str8_equals (s.update.table_name, str8_lit ("users")),
                "test[update] - Table name check");

    ASSERT_FMT (s.update.assign_col_count == 2,
                "test[update] - Assignment count wrong. Expected=2, Got=%d",
                s.update.assign_col_count);

    ASSERT_FMT (
        str8_equals (s.update.assignments[0].col_name, str8_lit ("name")),
        "test[update] - Assign 0 col");
    ASSERT_FMT (str8_equals (s.update.assignments[0].value, str8_lit ("Jane")),
                "test[update] - Assign 0 val");

    ASSERT_FMT (
        str8_equals (s.update.assignments[1].col_name, str8_lit ("age")),
        "test[update] - Assign 1 col");
    ASSERT_FMT (str8_equals (s.update.assignments[1].value, str8_lit ("30")),
                "test[update] - Assign 1 val");

    ASSERT_FMT (s.update.has_where == true, "test[update] - Should have WHERE");

    ASSERT_FMT (str8_equals (s.update.where_col.table_name, str8_lit ("users")),
                "test[update] - Where table part");
    ASSERT_FMT (str8_equals (s.update.where_col.col_name, str8_lit ("id")),
                "test[update] - Where col part");

    ASSERT_FMT (str8_equals (s.update.where_value, str8_lit ("1")),
                "test[update] - Where value check");

    printf ("PARSER: [update] All tests passed!\n");
}

int main ()
{
    test_create_stmt ();
    test_insert_stmt ();
    test_select_stmt ();
    test_delete_stmt ();
    test_update_stmt ();
    return 0;
}
