#include <stdio.h>

#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "config.h"

#include "src/log.h"
#include "src/request_handlers.h"
#include "src/socket_handler.h"
#include "src/tsup_msg.h"
#include "src/util.h"

enum LogLevel LOGGER_LEVEL = DEF_LOG_LEVEL;

void usage()
{
    puts("\
Usage: tsupq [OPTION...] CMD [ARGUMENT...]\n\
Send a request to the tsupd server.\n\
\n\
Options:\n\
  -h, --help                   Print this message\n\
  -q                           Decrease verbosity\n\
  -v                           Increase verbosity\n\
  -V, --version                Print version information and exit\n\
\n\
Commands:\n\
  check                        Check if tsupd server is running.\n\
                                 If no ID provided then list all processes\n\
  kill ID [SIGNAL]             Ask server to send signal to process(-es) \n\
                                 with id matching ID. \n\
                                 If no SIGNAL specified then it will send\n\
                                 SIGTERM to the process(-es).\n\
  list [ID]                    List processes with id matching ID.\n\
                                 If no ID provided then list all processes\n\
  shutdown                     Ask server to shutdown itself.\n\
  spawn ID COMMAND [ARG...]    Ask server to spawn process COMMAND [ARG...]\n\
                                 and assign it id=ID.\n\
");
}

void version()
{
    printf("tsupq version %s\n", VERSION);
}

bool response_status_to_bool(const struct TSupMsg* response)
{
    char *status = response->argv[response->argc - 1];
    return (strcmp(status, RESPONSE_SUCC) == 0);
}

void print_server_response(const struct TSupMsg *response)
{
    for (size_t i = 0; i < response->argc - 1; i++) {
        puts(response->argv[i]);
    }
}

bool verify_server_response(const struct TSupMsg* response)
{
    if (response == NULL) {
        log_msg(LOG_ERROR, "Failed to retrieve server response");
        return false;
    }

    if (response->argc == 0) {
        log_msg(LOG_ERROR, "Empty server response");
        return false;
    }

    const char *status = response->argv[response->argc - 1];
    if (
           (strcmp(status, RESPONSE_SUCC) != 0)
        && (strcmp(status, RESPONSE_FAIL) != 0)
    )
    {
        log_msg(LOG_ERROR, "Unrecognized response status: '%s'", status);
        return false;
    }

    return true;
}

struct TSupMsg* send_request(char *argv[], int offset)
{
    struct TSupMsg *result = NULL;
    int client_fd     = -1;
    char *socket_path = NULL;

    socket_path = get_socket_path();
    if (socket_path == NULL) {
        goto exit;
    }

    client_fd = open_client_socket(socket_path);
    if (client_fd == -1) {
        goto exit;
    }

    write_argv(client_fd, &argv[offset]);
    shutdown(client_fd, SHUT_WR);

    result = read_tsup_msg(client_fd);
    if (! verify_server_response(result))
    {
        free(result);
        result = NULL;
    }

    close(client_fd);

exit:
    free(socket_path);
    return result;
}

bool generic_cmd_handler(
    size_t nargs_min, size_t nargs_max,
    size_t start_idx, size_t nargs, char *argv[],
    bool verbose_error, bool print_response
)
{
    const char *cmd = argv[start_idx];

    if ((nargs < nargs_min) || (nargs > nargs_max))
    {
        usage();
        log_msg(
            LOG_ERROR, "Invalid number of arguments for command '%s'", cmd
        );
        return false;
    }

    struct TSupMsg *response = send_request(argv, start_idx);
    if (response == NULL)
    {
        if (verbose_error) {
            log_msg(LOG_ERROR, "Failed to execute command '%s'", cmd);
        }
        return false;
    }

    bool result = response_status_to_bool(response);
    if (print_response || (! result)) {
        print_server_response(response);
    }

    free_tsup_msg(response);

    return result;
}

bool handle_command(size_t start_idx, size_t nargs, char *argv[])
{
    const char *cmd = argv[start_idx];
    if (strcmp(cmd, "check") == 0) {
        return generic_cmd_handler(0, 1, start_idx, nargs, argv, false, false);
    }

    if (strcmp(cmd, "shutdown") == 0) {
        return generic_cmd_handler(0, 1, start_idx, nargs, argv, true, false);
    }

    if (strcmp(cmd, "list") == 0) {
        return generic_cmd_handler(0, 2, start_idx, nargs, argv, true, true);
    }

    if (strcmp(cmd, "spawn") == 0) {
        return generic_cmd_handler(3, -1, start_idx, nargs, argv, true, false);
    }

    if (strcmp(cmd, "kill") == 0) {
        return generic_cmd_handler(2, 3, start_idx, nargs, argv, true, false);
    }

    usage();
    log_msg(LOG_ERROR, "Unknown command: '%s'", cmd);
    return false;
}

int main(int argc, char* argv[])
{
    int arg_idx;

    for (arg_idx = 1; arg_idx < argc; arg_idx++)
    {
        if (! parse_global_argument(argv[arg_idx], usage, version)) {
            break;
        }
    }

    if (arg_idx >= argc) {
        usage();
        die_msg("No command specified");
    }

    int nargs = argc - arg_idx;

    if (handle_command(arg_idx, nargs, argv)) {
        return EXIT_SUCCESS;
    }
    else {
        return EXIT_FAILURE;
    }
}

