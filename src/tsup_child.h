#ifndef TSUP_CHILD_H
#define TSUP_CHILD_H

#include <sys/types.h>

struct TSupMsg;
struct TSupState;

struct TSupChild
{
    char  *id;
    pid_t pid;
    int   status_fd;

    struct event *child_event;

    struct TSupMsg   *command;
    struct TSupChild *next;
    struct TSupChild *prev;
};

/* Takes ownership of `command` on success. Transfers ownership of result. */
struct TSupChild* new_tsup_child(
    int statud_fd, struct TSupMsg *command, struct TSupState *state
);

void free_child(struct TSupChild *child);

#endif /* TSUP_CHILD_H */
