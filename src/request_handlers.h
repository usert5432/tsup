#ifndef REQUEST_HANDLERS_H
#define REQUEST_HANDLERS_H

#include <stdbool.h>
#include <stddef.h>

struct TSupMsg;
struct TSupState;

static const char RESPONSE_SUCC[] = "Success";
static const char RESPONSE_FAIL[] = "Fail";

void debug_print_request(const struct TSupMsg *request);
void pprint_child_cmdargs(const struct TSupMsg *command, int client_fd);

void handle_request(struct TSupState *state);

bool verify_nargs(
    struct TSupMsg *request, int client_fd, size_t argc_min, size_t argc_max
);

bool verify_id(struct TSupMsg *request, const char *id, int client_fd);

/* Takes ownership of `request` */
bool handle_client_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool check_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool list_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool spawn_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool kill_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool shutdown_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool empty_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

/* Takes ownership of `request` */
bool unknown_request(
    struct TSupMsg *request, int client_fd, struct TSupState *state
);

#endif /* REQUEST_HANDLERS_H */

