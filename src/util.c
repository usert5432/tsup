#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "src/log.h"

#define PATH_SIZE 4096

/* Returns pointer to an internal buffer on success. Should NOT be freed. */
char* create_tsup_workdir()
{
    static char path[PATH_SIZE];
    uid_t uid = getuid();

    int ret = snprintf(path, PATH_SIZE, "%s/tsup-%u", TMPDIR, uid);
    path[PATH_SIZE-1] = '\0';

    if (ret < 0) {
        log_errno(LOG_ERROR, "Failed to construct workdir path: ");
        return NULL;
    }

    if (ret >= PATH_SIZE-1) {
        log_msg(
            LOG_ERROR,
            "Failed to construct workdir path: File name is too long"
        );
        return NULL;
    }

    ret = mkdir(path, S_IRWXU);
    if ((ret < 0) && (errno != EEXIST)) {
        log_errno(LOG_ERROR, "Failed to create tsup workdir: '%s'", path);
        return NULL;
    }

    return path;
}

/* Transfers ownership of result. Returns NULL on error */
char* get_socket_path()
{
    const char *root  = create_tsup_workdir();
    if (root == NULL) {
        return NULL;
    }

    size_t path_len = strlen(root) + 1 + strlen(SOCKET_FILE) + 1;
    char *result    = malloc(path_len);
    if (result == NULL) {
        log_errno(LOG_ERROR, "Failed to allocate memory: ");
        return NULL;
    }

    int ret = snprintf(result, path_len, "%s/%s", root, SOCKET_FILE);
    if (ret < 0) {
        log_errno(LOG_ERROR, "Failed to construct socket path: ");
        goto error;
    }

    if (ret != path_len - 1) {
        log_msg(
            LOG_ERROR, "Some internal error happened: %d != %d",
            ret, path_len - 1
        );
        goto error;
    }

    return result;

error:
    free(result);
    return NULL;
}

bool parse_global_argument(
    const char *arg, void (*usage)(void), void (*version)(void)
)
{
    if (strcmp(arg, "-v") == 0)
    {
        increase_verbosity();
        return true;
    }

    if (strcmp(arg, "-q") == 0)
    {
        decrease_verbosity();
        return true;
    }

    if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--help") == 0))
    {
        usage();
        exit(EXIT_SUCCESS);
    }

    if ((strcmp(arg, "-V") == 0) || (strcmp(arg, "--version") == 0))
    {
        version();
        exit(EXIT_SUCCESS);
    }

    return false;
}

