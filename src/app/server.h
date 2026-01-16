#ifndef SERVER_H
#define SERVER_H

#include "../db/db.h"

#include <netinet/in.h>
#include <pthread.h>

#define PORT            9000
#define MAX_CONNECTIONS 256

void server_start ();

#endif /* SERVER_H */
