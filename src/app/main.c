#include "server.h"

// easier building
#include "../arena/arena.c"
#include "../btree/btree.c"
#include "../db/db.c"
#include "../executor/executor.c"
#include "../lexer/lexer.c"
#include "../pager/pager.c"
#include "../parser/parser.c"
#include "../str/str.c"
#include "../token/token.c"
#include "server.c"
#include "threadpool.c"

int main ()
{
    server_start ();
    return 0;
}
