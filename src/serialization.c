#include "serialization.h"

#include <ctype.h>
#include <signal.h>
#include <string.h>

#define TRY_DESERIALIZE_SIGNAL_AS(str,SIGNAL)   \
    if (strcmp(str, #SIGNAL) == 0) {            \
        return SIGNAL;                          \
    }

int deserialize_signal(const char *signal)
{
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGKILL);
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGTERM);
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGINT);
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGSTOP);
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGCONT);
    TRY_DESERIALIZE_SIGNAL_AS(signal, SIGHUP);

    return -1;
}

bool validate_id(const char *id)
{
    if (id == NULL) {
        return false;
    }

    for (const char *c = id; (*c) != '\0'; c++) {
        if (! isprint(*c)) {
            return false;
        }

        if ((*c == '"') || (*c == '\'') || (*c == ',')) {
            return false;
        }
    }

    return true;
}

