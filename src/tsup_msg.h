#ifndef TSUP_MSG_H
#define TSUP_MSG_H

#include <stddef.h>

struct TSupMsg
{
    char   *msg;        /* Contiguous buffer of null terminated strings */
    size_t msg_size;    /* Number of bytes in a buffer `msg` */

    /* NOTE: argv holds pointers to parts of msg. It does not own them */
    char **argv;
    size_t argc;
};

/* Transfers ownership of result */
struct TSupMsg* new_tsup_msg();

/* Transfers ownership of result */
struct TSupMsg* read_tsup_msg(int fd);

void free_tsup_msg(struct TSupMsg *command);

/* Transfers ownership of result. Sets `nread`. */
char   *read_until_eof(int fd, size_t *nread);
size_t count_number_of_msg_tokens(const char *msg, size_t msg_size);

/* Transfers ownership of result. Sets `argc`. */
char   **split_msg_into_argv(char *msg, size_t msg_size, size_t *argc);

void write_argv(int fd, char **argv);
void write_str_with_null_end(int fd, const char *fmt, ...);

#endif /* TSUP_MSG_H */
