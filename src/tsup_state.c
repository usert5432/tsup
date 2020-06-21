#include "tsup_state.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <event2/event.h>

#include "src/events.h"
#include "src/tsup_child.h"

/* Takes ownership of `server_fd` on success. Transfers ownership of result */
struct TSupState* new_tsup_state(int server_fd)
{
    struct TSupState *result = malloc(sizeof(struct TSupState));
    if (result == NULL) {
        goto error;
    }

    result->server_fd = server_fd;
    result->n_child   = 0;
    result->children  = NULL;

    if (! setup_server_event(result)) {
        goto error;
    }

    return result;

error:
    perror("Failed to create TSupState");
    free(result);
    return NULL;
}

void send_term_to_children(struct TSupState *state)
{
    struct TSupChild *child = state->children;
    while (child != NULL)
    {
        kill(child->pid, SIGTERM);
        child = child->next;
    }
}

void reap_children(struct TSupState *state)
{
    struct TSupChild *child = state->children;
    while (child != NULL)
    {
        struct TSupChild *curr = child;
        child = child->next;

        free_child(curr);
    }
}

void free_tsup_state(struct TSupState *state, int kill_timeout)
{
    if (state == NULL) {
        return;
    }

    send_term_to_children(state);

    if (kill_timeout > 0) {
        sleep(kill_timeout);
    }

    reap_children(state);

    event_free(state->server_event);
    event_base_free(state->eb);

    close(state->server_fd);

    free(state);
}

/* Takes ownership of `child` */
void add_child(struct TSupChild *child, struct TSupState *state)
{
    if (state->children == NULL) {
        state->children = child;
    }
    else {
        struct TSupChild *last_child = state->children;
        while (last_child->next != NULL) {
            last_child = last_child->next;
        }

        last_child->next = child;
        child->prev = last_child;
    }

    (state->n_child)++;
}

