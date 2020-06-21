#include <stdio.h>

#include <sys/file.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "config.h"

#include "src/tsup_state.h"
#include "src/events.h"
#include "src/log.h"
#include "src/socket_handler.h"
#include "src/util.h"

enum LogLevel LOGGER_LEVEL = DEF_LOG_LEVEL;

static const char LOCK_EXT[] = ".lock";

void usage()
{
    puts("\
Usage: tsupd [OPTION...]\n\
Start tsup server daemon.\n\
\n\
Options:\n\
  -h, --help                   Show this help message\n\
  -q                           Decrease verbosity\n\
  -v                           Increase verbosity\n\
  -V, --version                Print version information and exit\n\
\n"
);
}

void version()
{
    printf("tsupd version %s\n", VERSION);
}

/* Returns flock'ed fd on success, -1 on failure */
int acquire_socket_guard(const char *socket_path)
{
    int result = -1;
    char *path = malloc(strlen(socket_path) + strlen(LOCK_EXT) + 1);
    if (path == NULL) {
        log_errno(LOG_ERROR, "Failed to allocate memory: ");
        goto exit;
    }

    strcat(strcpy(path, socket_path), LOCK_EXT);

    result = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (result == -1) {
        log_errno(LOG_ERROR, "Failed to open lock file '%s': ", path);
        goto exit;
    }

    if (flock(result, LOCK_EX | LOCK_NB) == -1)
    {
        log_errno(LOG_ERROR, "Failed to acquire lock on '%s': ", path);
        close(result);
        result = -1;
    }

exit:
    free(path);
    return result;
}

void release_lock(int fd)
{
    if (fd == -1) {
        return;
    }

    flock(fd, LOCK_UN);
    close(fd);
}

void start_server()
{
    log_msg(LOG_INFO, "Starting server...");

    int lock_fd   = -1;
    int server_fd = -1;
    struct TSupState *state = NULL;

    char *socket_path = get_socket_path();
    if (socket_path == NULL) {
        goto exit_nolock;
    }

    lock_fd = acquire_socket_guard(socket_path);
    if (lock_fd == -1) {
        log_msg(LOG_INFO, "Check if another instance of tsupd is running");
        goto exit_nolock;
    }

    server_fd = open_server_socket(socket_path);
    if (server_fd == -1) {
        goto exit;
    }

    state = new_tsup_state(server_fd);
    if (state == NULL) {
        close(server_fd);
        goto exit;
    }

    server_event_loop(state);

exit:
    free_tsup_state(state, KILL_TIMEOUT);
    unlink(socket_path);

exit_nolock:
    free(socket_path);
    release_lock(lock_fd);
}

int main(int argc, char* argv[])
{
    int arg_idx;

    for (arg_idx = 1; arg_idx < argc; arg_idx++)
    {
        if (! parse_global_argument(argv[arg_idx], usage, version)) {
            usage();
            log_msg(LOG_ERROR, "Unknown argument: '%s'", argv[arg_idx]);
            exit(EXIT_FAILURE);
        }
    }

    start_server();
}

