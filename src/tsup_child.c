#include "tsup_child.h"

#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <event2/event.h>

#include "src/events.h"
#include "src/tsup_msg.h"

/* Takes ownership of `command` on success. Transfers ownership of result. */
struct TSupChild* new_tsup_child(
    int status_fd, struct TSupMsg *command, struct TSupState *state
)
{
    struct TSupChild *result = malloc(sizeof(struct TSupChild));
    if (result == NULL) {
        goto error;
    }

    result->status_fd = status_fd;
    result->next      = NULL;
    result->prev      = NULL;
    result->pid       = 0;
    result->id        = command->argv[1];
    result->command   = command;

    if (! setup_child_event(result, state)) {
        goto error;
    }

    return result;

error:
    free(result);
    return NULL;
}

void free_child(struct TSupChild *child)
{
    if (child == NULL) {
        return;
    }

    event_free(child->child_event);
    close(child->status_fd);

    if (child->pid > 0)
    {
        kill   (child->pid, SIGKILL);
        waitpid(child->pid, NULL, 0);
    }

    struct TSupChild *prev = child->prev;
    struct TSupChild *next = child->next;

    if (prev != NULL) {
        prev->next = next;
    }

    if (next != NULL) {
        next->prev = prev;
    }

    free_tsup_msg(child->command);
    free(child);
}

