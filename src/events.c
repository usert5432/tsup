#include "events.h"

#include <signal.h>
#include <stdio.h>
#include <event2/event.h>

#include "src/log.h"
#include "src/request_handlers.h"
#include "src/tsup_child.h"
#include "src/tsup_state.h"

/* Transfers ownership of result */
struct event* setup_signal_handler(struct TSupState *state, int signal)
{
    struct event *ev = event_new(
        state->eb, signal, EV_SIGNAL, cb_signal, state
    );

    if (ev == NULL) {
        goto error;
    }

    if (event_add(ev, NULL) != 0) {
        goto error;
    }

    return ev;

error:
    event_free(ev);
    return NULL;
}

void cb_signal(evutil_socket_t fd, short events, void *arg)
{
    struct TSupState *state = arg;
    event_base_loopbreak(state->eb);
    log_msg(LOG_INFO, "Received signal. Shutting Down...");
}

void cb_server_event(evutil_socket_t fd, short events, void *arg)
{
    struct TSupState *state = arg;

    log_msg(LOG_DEBUG, "Server Event");
    handle_request(state);
}

void cb_child_event(evutil_socket_t fd, short events, void *arg)
{
    struct TSupState *state = arg;

    log_msg(LOG_DEBUG, "Child Event");

    struct TSupChild *died_child = state->children;
    struct TSupChild *next_child = NULL;

    bool found_child = false;
    while (died_child != NULL)
    {
        if (died_child->status_fd == fd)
        {
            next_child = died_child->next;

            free_child(died_child);
            state->n_child--;

            found_child = true;
            break;
        }

        died_child = died_child->next;
    }

    if (state->children == died_child) {
        state->children = next_child;
    }

    if (! found_child) {
        die_err("Unable to find child that died. Likely a bug. Bailing out.");
    }
}

bool setup_server_event(struct TSupState *state)
{
    struct event_base *eb = NULL;
    struct event      *ev = NULL;

    eb = event_base_new();
    if (eb == NULL) {
        goto error;
    }

    ev = event_new(
        eb, state->server_fd, EV_READ | EV_PERSIST, cb_server_event, state
    );
    if (ev == NULL) {
        goto error;
    }

    if (event_add(ev, NULL) != 0) {
        goto error;
    }

    state->eb = eb;
    state->server_event = ev;

    return true;

error:
    event_base_free(eb);
    event_free(ev);
    return false;
}

bool setup_child_event(struct TSupChild *child, struct TSupState *state)
{
    struct event *ev = NULL;

    ev = event_new(
        state->eb, child->status_fd, EV_READ | EV_PERSIST, cb_child_event,
        state
    );
    if (ev == NULL) {
        goto error;
    }

    if (event_add(ev, NULL) != 0) {
        goto error;
    }

    child->child_event = ev;

    return true;

error:
    event_free(ev);
    return false;
}

void server_event_loop(struct TSupState *state)
{
    signal(SIGPIPE, SIG_IGN);

    struct event *ev_term = setup_signal_handler(state, SIGTERM);
    struct event *ev_int  = setup_signal_handler(state, SIGINT);

    if ((ev_term == NULL) || (ev_int == NULL)) {
        log_msg(LOG_ERROR, "Failed to setup signal handlers. Aborting.");
    }
    else {
        event_base_dispatch(state->eb);
    }

    event_free(ev_term);
    event_free(ev_int);
}

