#include "tsup_msg.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "src/log.h"

/* Transfers ownership of result */
struct TSupMsg* new_tsup_msg()
{
    struct TSupMsg* result = malloc(sizeof(struct TSupMsg));
    if (result == NULL) {
        return NULL;
    }

    result->msg      = NULL;
    result->msg_size = 0;

    result->argv = NULL;
    result->argc = 0;

    return result;
}

void free_tsup_msg(struct TSupMsg *command)
{
    if (command == NULL) {
        return;
    }

    free(command->argv);
    free(command->msg);
    free(command);
}

/* Transfers ownership of result. Sets `nread`. */
char *read_until_eof(int fd, size_t *nread)
{
    int  ret;
    char *result = NULL;
    char buffer[RECV_BUFFER_SIZE];

    *nread = 0;

    for (;;)
    {
        ret = read(fd, buffer, RECV_BUFFER_SIZE);
        if (ret == -1) {
            log_errno(LOG_WARN, "Read error: ");
            goto error;
        }

        if (ret == 0) {
            break;
        }

        char *tmp = realloc(result, (*nread) + ret);
        if (tmp == NULL) {
            log_errno(LOG_WARN, "Read buffer realloc error");
            goto error;
        }
        result = tmp;

        memcpy(&result[(*nread)], buffer, ret);
        (*nread) += ret;
    }

    return result;

error:
    free(result);
    return NULL;
}

size_t count_number_of_msg_tokens(const char *msg, size_t msg_size)
{
    size_t result = 0;

    for (size_t i = 0; i < msg_size; i++)
    {
        if (msg[i] == '\0') {
            result++;
        }
    }

    return result;
}

/* Transfers ownership of result. Sets `argc`. */
/*
 * NOTE:
 *  - argv contains pointers to various locations in `msg`.
 *  - argv does NOT own those pointers.
 *  - argv[-1] == NULL
 */
char **split_msg_into_argv(char *msg, size_t msg_size, size_t *argc)
{
    (*argc)     = count_number_of_msg_tokens(msg, msg_size);
    char **argv = malloc(((*argc) + 1) * sizeof(char *));

    if (argv == NULL) {
        log_errno(LOG_WARN, "failed to allocate memory for the argument list");
        return argv;
    }

    argv[0]     = msg;
    argv[*argc] = NULL;

    for (size_t msg_idx = 0, argv_idx = 0; msg_idx < msg_size; msg_idx++)
    {
        if (msg[msg_idx] == '\0')
        {
            msg_idx++; argv_idx++;

            if ((argv_idx < (*argc)) && (msg_idx < msg_size)) {
                argv[argv_idx] = &msg[msg_idx];
            }
        }
    }

    return argv;
}

/* Transfers ownership of result */
struct TSupMsg* read_tsup_msg(int fd)
{
    struct TSupMsg* result = new_tsup_msg();
    if (result == NULL) {
        log_errno(LOG_WARN, "Failed to malloc");
        goto error;
    }

    result->msg = read_until_eof(fd, &result->msg_size);
    if (result->msg == NULL) {
        log_msg(LOG_WARN, "Failed to read string from the fd");
        goto error;
    }

    result->argv = split_msg_into_argv(
        result->msg, result->msg_size, &result->argc
    );
    if (result->argv == NULL) {
        log_msg(LOG_WARN, "Failed to split msg into argv");
        goto error;
    }

    return result;

error:
    free_tsup_msg(result);
    return NULL;
}

void write_argv(int fd, char **argv)
{
    for (char **arg = argv; *arg != NULL; arg++)
    {
        int ret = write(fd, *arg, strlen(*arg) + 1);
        if (ret == -1) {
            log_errno(LOG_WARN, "Failed to write to the fd");
        }
    }
}

void write_str_with_null_end(int fd, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vdprintf(fd, fmt, args);
    int ret = write(fd, "\0", 1);

    if (ret == -1) {
        log_errno(LOG_WARN, "Failed to write to client");
    }

    va_end(args);
}

