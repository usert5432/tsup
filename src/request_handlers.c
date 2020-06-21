#include "request_handlers.h"

#include <errno.h>
#include <fnmatch.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <event2/event.h>

#include "src/log.h"
#include "src/proc.h"
#include "src/request_handlers.h"
#include "src/socket_handler.h"
#include "src/tsup_msg.h"
#include "src/tsup_child.h"
#include "src/tsup_state.h"
#include "src/serialization.h"

#define REQUEST_ERR_VERB(FD,CMD,FMT,...)                \
    do {                                                \
        log_msg(LOG_WARN, "Request '%s' error: " FMT, CMD, __VA_ARGS__); \
        write_str_with_null_end(FD, FMT, __VA_ARGS__);  \
    } while (0)

#define REQUEST_ERR(FD,...)                             \
    do {                                                \
        log_msg(LOG_WARN, __VA_ARGS__);                 \
        write_str_with_null_end(FD, __VA_ARGS__);       \
    } while (0)


void debug_print_request(const struct TSupMsg *request)
{
    if (LOG_DEBUG < LOGGER_LEVEL) {
        return;
    }

    log_msg(LOG_DEBUG, "Handling request:");
    for (size_t i = 0; i < request->argc; i++) {
        log_msg(LOG_DEBUG, "    argv[%lu] = %s", i, request->argv[i]);
    }
}

void pprint_child_cmdargs(const struct TSupMsg *command, int client_fd)
{
    /* For child argv[0] == "spawn", and argv[1] == CHILD_ID  */
    dprintf(client_fd, "\"");

    for (size_t i = 2; i < command->argc - 1; i++)
    {
        if (i > 2) {
            dprintf(client_fd, " ");
        }

        dprintf(client_fd, "'%s'", command->argv[i]);
    }

    write_str_with_null_end(
        client_fd, " '%s'\"", command->argv[command->argc-1]
    );
}

bool verify_nargs(
    struct TSupMsg *request, int client_fd, size_t argc_min, size_t argc_max
)
{
    const char *cmd = request->argv[0];
    if (request->argc < argc_min)
    {
        REQUEST_ERR(
            client_fd, "Too few arguments for '%s' request %d < %lu",
            cmd, request->argc, argc_min
        );

        return false;
    }

    if (request->argc > argc_max)
    {
        REQUEST_ERR(
            client_fd, "Too many arguments for '%s' request %d > %lu",
            cmd, request->argc, argc_max
        );

        return false;
    }

    return true;
}

bool verify_id(struct TSupMsg *request, const char *id, int client_fd)
{
    bool result = validate_id(id);
    if (! result) {
        REQUEST_ERR_VERB(
            client_fd, request->argv[0], "Invalid ID '%s'", id
        );
    }

    return result;
}

/* Takes ownership of `request` */
bool list_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    bool result = false;
    char *id    = NULL;
    size_t n_children = 0;

    if (! verify_nargs(request, client_fd, 0, 2)) {
        goto exit;
    }

    if (request->argc > 1)
    {
        id = request->argv[1];
        if (! verify_id(request, id, client_fd)) {
            goto exit;
        }
    }

    struct TSupChild *child = state->children;
    for (; child != NULL; child = child->next)
    {
        if ((id != NULL) && (fnmatch(id, child->id, 0) != 0)) {
            continue;
        }

        dprintf(
            client_fd, "ID: \"%s\", PID: %d, CMD: ", child->id, child->pid
        );
        pprint_child_cmdargs(child->command, client_fd);

        n_children++;
    }

    result = true;

exit:
    free_tsup_msg(request);
    return result;
}

/* Takes ownership of `request` */
bool spawn_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    if (! verify_nargs(request, client_fd, 3, -1)) {
        goto error;
    }

    const char *id = request->argv[1];

    if (! verify_id(request, id, client_fd)) {
        goto error;
    }

    log_msg(
        LOG_INFO, "Request to spawn process with ID='%s' : '%s'",
        id, request->argv[2]
    );

    /* NOTE: transfer request ownership on success */
    bool result = spawn_process(request, state);
    if (! result) {
        write_str_with_null_end(
            client_fd, "Failed to spawn process '%s': %s", request->argv[2],
            strerror(errno)
        );
        goto error;
    }

    return result;

error:
    free_tsup_msg(request);
    return false;
}

/* Takes ownership of `request` */
bool kill_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    bool result = false;
    if (! verify_nargs(request, client_fd, 2, 3)) {
        goto exit;
    }

    char *id = request->argv[1];
    if (! verify_id(request, id, client_fd)) {
        goto exit;
    }

    int signal = SIGTERM;

    if (request->argc == 3)
    {
        signal = deserialize_signal(request->argv[2]);
        if (signal == -1) {
            REQUEST_ERR_VERB(
                client_fd, request->argv[0],
                "Unknown signal '%s'", request->argv[2]
            );
            goto exit;
        }
    }

    log_msg(
        LOG_INFO,
        "Request to kill processes with ID '%s' and signal '%s'",
        id, (request->argc == 3) ? request->argv[2] : "SIGTERM"
    );

    kill_process_by_id(id, signal, state);
    result = true;

exit:
    free_tsup_msg(request);
    return result;
}

/* Takes ownership of `request` */
bool empty_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    REQUEST_ERR(client_fd, "Empty request");
    free_tsup_msg(request);
    return false;
}

/* Takes ownership of `request` */
bool unknown_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    REQUEST_ERR(client_fd, "Unknown request: %s", request->argv[0]);
    free_tsup_msg(request);
    return false;
}

/* Takes ownership of `request` */
bool check_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    free_tsup_msg(request);
    return true;
}

/* Takes ownership of `request` */
bool shutdown_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    free_tsup_msg(request);
    log_msg(LOG_INFO, "Received shut down request. Shutting down...");

    event_base_loopbreak(state->eb);
    return true;
}

/* Takes ownership of `request` */
bool handle_client_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
)
{
    if (request->argc == 0) {
        return empty_request(request, client_fd, state);
    }

    char *cmd = request->argv[0];
    debug_print_request(request);

    if (strcmp(cmd, "check") == 0) {
        return check_request(request, client_fd, state);
    }

    if (strcmp(cmd, "list") == 0) {
        return list_request(request, client_fd, state);
    }

    if (strcmp(cmd, "spawn") == 0) {
        return spawn_request(request, client_fd, state);
    }

    if (strcmp(cmd, "kill") == 0) {
        return kill_request(request, client_fd, state);
    }

    if (strcmp(cmd, "shutdown") == 0) {
        return shutdown_request(request, client_fd, state);
    }

    return unknown_request(request, client_fd, state);
}

void handle_request(struct TSupState *state)
{
    int client_fd = accept_client_connection(state->server_fd);
    struct TSupMsg *request = read_tsup_msg(client_fd);

    if (request == NULL) {
        log_msg(
            LOG_WARN, "Aborting request handling due to a failure to parse it"
        );
    }
    else {
        /* Transfer ownership of request */
        if (handle_client_request(request, client_fd, state)) {
            write_str_with_null_end(client_fd, RESPONSE_SUCC);
        }
        else {
            write_str_with_null_end(client_fd, RESPONSE_FAIL);
        }
    }

    close_socket(client_fd);
}

