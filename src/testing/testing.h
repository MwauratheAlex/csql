#ifndef TESTING_H
#define TESTING_H

#include <stdlib.h>

#define ASSERT_FMT(cond, fmt, ...)                                             \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            fprintf (stderr, "ASSERTION FAILED: %s:%d\n", __FILE__, __LINE__); \
            fprintf (stderr, "   " fmt "\n", ##__VA_ARGS__);                   \
            exit (1);                                                          \
        }                                                                      \
    } while (0)

#endif /* TESTING_H */
