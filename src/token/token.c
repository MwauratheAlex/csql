#include "token.h"

Token token_new (TokenType type, str8 literal)
{
    Token t;
    t.type = type;
    t.literal = literal;
    return t;
}

const char *token_type_to_string (TokenType type)
{
    switch (type)
    {
    case TOKEN_ILLEGAL:
        return "ILLEGAL";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_IDENT:
        return "IDENT";
    case TOKEN_INT:
        return "INT";
    case TOKEN_STRING:
        return "STRING";

    case TOKEN_ASSIGN:
        return "ASSIGN"; // "="
    case TOKEN_PLUS:
        return "PLUS"; // "+"
    case TOKEN_COMMA:
        return "COMMA"; // ","
    case TOKEN_SEMICOLON:
        return "SEMICOLON"; // ";"
    case TOKEN_DOT:
        return "DOT"; // "."
    case TOKEN_LPAREN:
        return "LPAREN"; // "("
    case TOKEN_RPAREN:
        return "RPAREN"; // ")"
    case TOKEN_ASTERISK:
        return "ASTERISK"; // "*"

    case TOKEN_CREATE:
        return "CREATE";
    case TOKEN_TABLE:
        return "TABLE";
    case TOKEN_INSERT:
        return "INSERT";
    case TOKEN_INTO:
        return "INTO";
    case TOKEN_VALUES:
        return "VALUES";
    case TOKEN_SELECT:
        return "SELECT";
    case TOKEN_FROM:
        return "FROM";
    case TOKEN_UPDATE:
        return "UPDATE";
    case TOKEN_SET:
        return "SET";
    case TOKEN_DELETE:
        return "DELETE";
    case TOKEN_WHERE:
        return "WHERE";
    case TOKEN_AND:
        return "AND";
    case TOKEN_OR:
        return "OR";
    case TOKEN_JOIN:
        return "JOIN";
    case TOKEN_ON:
        return "ON";
    case TOKEN_PRIMARY:
        return "PRIMARY";
    case TOKEN_KEY:
        return "KEY";
    case TOKEN_UNIQUE:
        return "UNIQUE";

    case TOKEN_INT_TYPE:
        return "INT_TYPE";
    case TOKEN_TEXT_TYPE:
        return "TEXT_TYPE";

    default:
        return "UNKNOWN";
    }
}
