#ifndef LEXER_H
#define LEXER_H
#include "../token/token.h"
#include "../str/str.h"

typedef struct
{
    str8 input;
    int position;
    int read_position;
    char ch;
} Lexer;

void
lexer_init (Lexer *l, const char *input);

Token
lexer_next_token (Lexer *l);

#endif /* LEXER_H */
