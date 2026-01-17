#include "../testing/testing.h"
#include "assert.h"
#include "lexer.h"
#include "stdio.h"

// unity build
#include "../arena/arena.c"
#include "../str/str.c"
#include "../token/token.c"
#include "lexer.c"

int main ()
{
    char *input = "CREATE TABLE int text ( ) , ; . * = "
                  "PRIMARY KEY UNIQUE INSERT INTO VALUES "
                  "SELECT FROM UPDATE SET DELETE "
                  "WHERE AND OR JOIN ON "
                  "123 'hello' my_var #";

    typedef struct
    {
        TokenType expected_type;
        str8 expectedLiteral;
    } Test;

    Test tests[] = {
        {TOKEN_CREATE, str8_lit ("CREATE")},
        {TOKEN_TABLE, str8_lit ("TABLE")},
        {TOKEN_INT_TYPE, str8_lit ("int")},
        {TOKEN_TEXT_TYPE, str8_lit ("text")},

        {TOKEN_LPAREN, str8_lit ("(")},
        {TOKEN_RPAREN, str8_lit (")")},
        {TOKEN_COMMA, str8_lit (",")},
        {TOKEN_SEMICOLON, str8_lit (";")},
        {TOKEN_DOT, str8_lit (".")},
        {TOKEN_ASTERISK, str8_lit ("*")},
        {TOKEN_ASSIGN, str8_lit ("=")},

        {TOKEN_PRIMARY, str8_lit ("PRIMARY")},
        {TOKEN_KEY, str8_lit ("KEY")},
        {TOKEN_UNIQUE, str8_lit ("UNIQUE")},

        {TOKEN_INSERT, str8_lit ("INSERT")},
        {TOKEN_INTO, str8_lit ("INTO")},
        {TOKEN_VALUES, str8_lit ("VALUES")},

        {TOKEN_SELECT, str8_lit ("SELECT")},
        {TOKEN_FROM, str8_lit ("FROM")},

        {TOKEN_UPDATE, str8_lit ("UPDATE")},
        {TOKEN_SET, str8_lit ("SET")},
        {TOKEN_DELETE, str8_lit ("DELETE")},

        {TOKEN_WHERE, str8_lit ("WHERE")},
        {TOKEN_AND, str8_lit ("AND")},
        {TOKEN_OR, str8_lit ("OR")},
        {TOKEN_JOIN, str8_lit ("JOIN")},
        {TOKEN_ON, str8_lit ("ON")},

        {TOKEN_INT, str8_lit ("123")},
        {TOKEN_STRING, str8_lit ("hello")},
        {TOKEN_IDENT, str8_lit ("my_var")},

        {TOKEN_ILLEGAL, str8_lit ("#")},
        {TOKEN_EOF, str8_lit ("")},
    };

    Lexer l;
    lexer_init (&l, input);

    int test_count = sizeof (tests) / sizeof (tests[0]);

    for (int i = 0; i < test_count; i++)
    {
        Token t = lexer_next_token (&l);

        ASSERT_FMT (t.type == tests[i].expected_type,
                    "tests[%d] - tokentype wrong. expected=%s, got=%s", i,
                    token_type_to_string (tests[i].expected_type),
                    token_type_to_string (t.type));

        ASSERT_FMT (str8_equals (t.literal, tests[i].expectedLiteral),
                    "tests[%d] - tokenliteral wrong. expected=%.*s, got=%.*s",
                    i, STR_FMT (tests[i].expectedLiteral), STR_FMT (t.literal));
    }

    printf ("LEXER: All %d tests passed!\n", test_count);
}
