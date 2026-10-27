/* The tests' one assertion: CHECK(cond, printf-style message), counting failures in fails. */
#pragma once

#include <stdio.h>

static int fails;
#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (!(cond)) {                                      \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);     \
            printf(__VA_ARGS__);                            \
            printf("\n");                                   \
            fails++;                                        \
        }                                                   \
    } while (0)
