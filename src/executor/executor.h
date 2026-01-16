#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "../db/db.h"
#include "../parser/parser.h"

typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_DB_FULL,

    EXECUTE_TABLE_EXISTS,
    EXECUTE_TABLE_FULL,
    EXECUTE_TABLE_NOT_EXISTS,
    EXECUTE_TABLE_COL_COUNT_MISMATCH,

    EXECUTE_COL_NOT_FOUND,

    EXECUTE_DUPLICATE_KEY,

    EXECUTE_FAIL
} ExecuteResult;

ExecuteResult execute_statement (Statement *stmt, Database *db, int client_fd);

#endif /* EXECUTOR_H */
