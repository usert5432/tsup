#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>
#include <event2/util.h>

struct TSupState;
struct TSupChild;

bool setup_server_event(struct TSupState *state);
bool setup_child_event (struct TSupChild *child, struct TSupState *state);

void server_event_loop(struct TSupState *state);

void cb_child_event (evutil_socket_t fd, short events, void *arg);
void cb_server_event(evutil_socket_t fd, short events, void *arg);
void cb_signal      (evutil_socket_t fd, short events, void *arg);

/* Transfers ownership of result */
struct event* setup_signal_handler(struct TSupState *state, int signal);

#endif /* EVENTS_H */
