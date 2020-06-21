#ifndef TSUP_STATE_H
#define TSUP_STATE_H

#include <stddef.h>

struct TSupChild;

struct TSupState
{
    int server_fd;

    size_t n_child;
    struct TSupChild *children;

    struct event_base *eb;
    struct event      *server_event;

};

/* Takes ownership of `server_fd` on success. Transfers ownership of result */
struct TSupState* new_tsup_state(int server_fd);
void free_tsup_state(struct TSupState *state, int kill_timeout);

/* Takes ownership of `child` */
void add_child(struct TSupChild *child, struct TSupState *state);

void send_term_to_children(struct TSupState *state);
void reap_children(struct TSupState *state);

#endif /* TSUP_STATE_H */
