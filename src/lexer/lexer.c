#include "lexer.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

static void lexer_read_char (Lexer *l);
static void lexer_skip_whitespace (Lexer *l);
static Token lexer_read_identifier (Lexer *l);
static Token lexer_read_number (Lexer *l);
static Token lexer_read_string (Lexer *l);
static TokenType lookup_ident (str8 ident);

void lexer_init (Lexer *l, const char *input)
{
    l->input = str8_from_cstr (input);

    l->position = 0;
    l->read_position = 0;
    l->ch = 0;

    lexer_read_char (l);
}

Token lexer_next_token (Lexer *l)
{
    Token t;
    lexer_skip_whitespace (l);

    str8 lit_char = {(uint8_t *) &l->input.str[l->position], 1};
    str8 lit_empty = {NULL, 0};

    switch (l->ch)
    {
    case '.':
        t = token_new (TOKEN_DOT, lit_char);
        break;
    case '=':
        t = token_new (TOKEN_ASSIGN, lit_char);
        break;
    case ';':
        t = token_new (TOKEN_SEMICOLON, lit_char);
        break;
    case '(':
        t = token_new (TOKEN_LPAREN, lit_char);
        break;
    case ')':
        t = token_new (TOKEN_RPAREN, lit_char);
        break;
    case ',':
        t = token_new (TOKEN_COMMA, lit_char);
        break;
    case '*':
        t = token_new (TOKEN_ASTERISK, lit_char);
        break;
    case 0:
        t = token_new (TOKEN_EOF, lit_empty);
        break;
    case '\'':
        return lexer_read_string (l);
    default:
        if (isalpha (l->ch) || l->ch == '_')
        {
            return lexer_read_identifier (l);
        }
        else if (isdigit (l->ch))
        {
            return lexer_read_number (l);
        }
        else
        {
            t = token_new (TOKEN_ILLEGAL, lit_char);
        }
    }

    lexer_read_char (l);

    return t;
}

void lexer_read_char (Lexer *l)
{
    if (l->read_position >= l->input.len)
    {
        l->ch = 0;
    }
    else
    {
        l->ch = l->input.str[l->read_position];
    }

    l->position = l->read_position;
    l->read_position++;
}

static void lexer_skip_whitespace (Lexer *l)
{
    while (l->ch == ' ' || l->ch == '\t' || l->ch == '\n' || l->ch == '\r')
    {
        lexer_read_char (l);
    }
}

static Token lexer_read_identifier (Lexer *l)
{
    int start = l->position;
    while (isalpha (l->ch) || l->ch == '_')
    {
        lexer_read_char (l);
    }

    str8 literal = str8_from_range ((uint8_t *) (l->input.str + start),
                                    (uint8_t *) (l->input.str + l->position));
    TokenType type = lookup_ident (literal);

    return token_new (type, literal);
}

static Token lexer_read_number (Lexer *l)
{
    int start = l->position;
    while (isdigit (l->ch))
    {
        lexer_read_char (l);
    }

    str8 literal = str8_from_range ((uint8_t *) (l->input.str + start),
                                    (uint8_t *) (l->input.str + l->position));

    return token_new (TOKEN_INT, literal);
}

static Token lexer_read_string (Lexer *l)
{
    lexer_read_char (l); // skip opening quote

    int start = l->position;
    while (l->ch != '\'' && l->ch != 0)
    {
        lexer_read_char (l);
    }
    str8 literal = str8_from_range ((uint8_t *) (l->input.str + start),
                                    (uint8_t *) (l->input.str + l->position));

    if (l->ch == '\'')
        lexer_read_char (l); // skip closing quote

    return token_new (TOKEN_STRING, literal);
}

static TokenType lookup_ident (str8 ident)
{
    if (str8_equals (ident, str8_lit ("CREATE")))
        return TOKEN_CREATE;
    if (str8_equals (ident, str8_lit ("TABLE")))
        return TOKEN_TABLE;
    if (str8_equals (ident, str8_lit ("INSERT")))
        return TOKEN_INSERT;
    if (str8_equals (ident, str8_lit ("INTO")))
        return TOKEN_INTO;
    if (str8_equals (ident, str8_lit ("VALUES")))
        return TOKEN_VALUES;
    if (str8_equals (ident, str8_lit ("SELECT")))
        return TOKEN_SELECT;
    if (str8_equals (ident, str8_lit ("FROM")))
        return TOKEN_FROM;
    if (str8_equals (ident, str8_lit ("UPDATE")))
        return TOKEN_UPDATE;
    if (str8_equals (ident, str8_lit ("SET")))
        return TOKEN_SET;
    if (str8_equals (ident, str8_lit ("DELETE")))
        return TOKEN_DELETE;

    if (str8_equals (ident, str8_lit ("WHERE")))
        return TOKEN_WHERE;
    if (str8_equals (ident, str8_lit ("AND")))
        return TOKEN_AND;
    if (str8_equals (ident, str8_lit ("OR")))
        return TOKEN_OR;
    if (str8_equals (ident, str8_lit ("JOIN")))
        return TOKEN_JOIN;
    if (str8_equals (ident, str8_lit ("ON")))
        return TOKEN_ON;

    if (str8_equals (ident, str8_lit ("PRIMARY")))
        return TOKEN_PRIMARY;
    if (str8_equals (ident, str8_lit ("KEY")))
        return TOKEN_KEY;
    if (str8_equals (ident, str8_lit ("UNIQUE")))
        return TOKEN_UNIQUE;

    if (str8_equals (ident, str8_lit ("int")))
        return TOKEN_INT_TYPE;
    if (str8_equals (ident, str8_lit ("text")))
        return TOKEN_TEXT_TYPE;

    return TOKEN_IDENT;
}
