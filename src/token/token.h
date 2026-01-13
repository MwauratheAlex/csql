#ifndef TOKEN_H
#define TOKEN_H
#include "../str/str.h"

typedef enum
{
    TOKEN_ILLEGAL,
    TOKEN_EOF,
    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_ASTERISK,
    TOKEN_INTO,
    TOKEN_VALUES,
    TOKEN_FROM,
    TOKEN_TABLE,
    TOKEN_INT_TYPE,
    TOKEN_TEXT_TYPE,

    TOKEN_CREATE,
    TOKEN_INSERT,
    TOKEN_SELECT,
    TOKEN_UPDATE,
    TOKEN_SET,
    TOKEN_DELETE,
    TOKEN_WHERE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_JOIN,
    TOKEN_ON,
    TOKEN_PRIMARY,
    TOKEN_KEY,
    TOKEN_UNIQUE
} TokenType;

typedef struct
{
    TokenType type;
    str8 literal;
} Token;

const char *token_type_to_string (TokenType type);
Token token_new (TokenType type, str8 literal);

#endif /* TOKEN_H */
