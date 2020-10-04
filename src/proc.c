#include "proc.h"

#include <fnmatch.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "src/log.h"
#include "src/tsup_child.h"
#include "src/tsup_msg.h"
#include "src/tsup_state.h"

bool block_int_term_signals(sigset_t *old_set)
{
    sigset_t new_set;

    if (sigemptyset(&new_set) != 0) {
        goto error;
    }

    if (sigaddset(&new_set, SIGINT) != 0) {
        goto error;
    }

    if (sigaddset(&new_set, SIGTERM) != 0) {
        goto error;
    }

    if (sigprocmask(SIG_BLOCK, &new_set, old_set) != 0) {
        goto error;
    }

    return true;

error:
    log_errno(LOG_WARN, "Failed to block signals: ");
    return false;
}

bool restore_old_signal_mask(const sigset_t *old_set)
{
    if (sigprocmask(SIG_SETMASK, old_set, NULL) != 0) {
        log_errno(LOG_WARN, "Failed to restore old signal mask: ");
        return false;
    }
    else {
        return true;
    }
}

bool fork_process_child_func(char **argv, const sigset_t *old_set)
{
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT,  SIG_DFL);

    restore_old_signal_mask(old_set);

    int ret = execvp(argv[0], argv);

    if (ret == -1) {
        die_err("execvp");
    }

    return true;
}

/* Takes ownership of command on success */
bool spawn_process(struct TSupMsg *command, struct TSupState *state)
{
    int ret = 0;
    /*
     * status_pipe[0] -- read  end (parent)
     * status_pipe[1] -- write end (fork)
     */
    int status_pipe[2];

    ret = pipe(status_pipe);
    if (ret == -1) {
        log_errno(LOG_WARN, "Failed to create process status pipe: ");
        return false;
    }

    struct TSupChild *child = new_tsup_child(status_pipe[0], command, state);
    if (child == NULL) {
        log_errno(LOG_WARN, "Failed to create child: ");
        goto error;
    }

    sigset_t parent_signal_set;
    if (! block_int_term_signals(&parent_signal_set)) {
        goto error;
    }

    ret = fork();
    switch (ret)
    {
    case -1: /* Error */
        log_errno(LOG_WARN, "Failed to fork: ");
        /* Do NOT free command on error */
        child->command = NULL;
        restore_old_signal_mask(&parent_signal_set);
        goto error;

    case 0: /* Child process */
        close(status_pipe[0]);
        return fork_process_child_func(&command->argv[2], &parent_signal_set);

    default: /* Parent process */
        close(status_pipe[1]);
        child->pid = ret;
        add_child(child, state);

        restore_old_signal_mask(&parent_signal_set);
        return true;
    }

error:
    free_child(child);

    close(status_pipe[0]);
    close(status_pipe[1]);
    return false;
}

void kill_process_by_id(const char *id, int signal, struct TSupState *state)
{
    const struct TSupChild *child = state->children;
    while (child != NULL)
    {
        if (fnmatch(id, child->id, 0) == 0) {
            log_msg(
                LOG_DEBUG, "Found child ID match for kill. PID: %u", child->pid
            );
            kill(child->pid, signal);
        }
        child = child->next;
    }
}

