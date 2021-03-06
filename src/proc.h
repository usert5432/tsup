#ifndef PROC_H
#define PROC_H

#include <signal.h>
#include <stdbool.h>

struct TSupMsg;
struct TSupState;

/* Takes ownership of command on success */
bool spawn_process(struct TSupMsg *command, struct TSupState *state);
void kill_process_by_id(const char *id, int signal, struct TSupState *state);

bool fork_process_child_func(char **argv, const sigset_t *old_set);

#endif /* PROC_H */
