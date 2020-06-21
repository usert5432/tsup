#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

/* Returns pointer to an internal buffer on success. Should NOT be freed. */
char* create_tsup_workdir();

/* Transfers ownership of result. Returns NULL on error */
char* get_socket_path();

bool parse_global_argument(
    const char *arg, void (*usage)(void), void (*version)(void)
);

#endif /* UTIL_H */
