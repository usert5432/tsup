#include "socket_handler.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "config.h"
#include "src/log.h"
#include "src/tsup_msg.h"

int accept_client_connection(int server_fd)
{
    struct sockaddr_un sa;
    socklen_t sock_addr_size = sizeof(struct sockaddr_un);

    int client_fd = accept(
        server_fd, (struct sockaddr *) &sa, &sock_addr_size
    );
    if (client_fd == -1) {
        log_errno(LOG_WARN, "Failed to accept client connection: ");
    }

    return client_fd;
}

void close_socket(int socket_fd)
{
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
}

int setup_unix_socket(const char *path, struct sockaddr_un *sa)
{
    if (strlen(path) > sizeof(sa->sun_path) - 1) {
        log_msg(LOG_ERROR, "Socket file name is too long");
        return -1;
    }

    int result = socket(AF_UNIX, SOCK_STREAM, 0);
    if (result == -1) {
        log_errno(LOG_ERROR, "Failed to open unix socket: ");
        return -1;
    }

    memset(sa, 0, sizeof(struct sockaddr_un));
    sa->sun_family = AF_UNIX;
    strcpy(sa->sun_path, path);

    return result;
}

int open_server_socket(const char *path)
{
    int ret;
    struct sockaddr_un sa;

    unlink(path);

    int socket_fd = setup_unix_socket(path, &sa);
    if (socket_fd == -1) {
        return -1;
    }

    ret = bind(
        socket_fd, (const struct sockaddr *) &sa, sizeof(struct sockaddr_un)
    );
    if (ret == -1) {
        log_errno(
            LOG_ERROR, "Failed to bind socket to: '%s' : ", path
        );
        goto error;
    }

    ret = listen(socket_fd, SOCKET_BACKLOG);
    if (ret == -1) {
        log_errno(
            LOG_ERROR, "Failed to bring server socket into listening state."
        );
        goto error;
    }

    return socket_fd;

error:
    close(socket_fd);
    return -1;
}

int open_client_socket(const char *path)
{
    int ret;
    struct sockaddr_un sa;

    int socket_fd = setup_unix_socket(path, &sa);
    if (socket_fd == -1) {
        return -1;
    }

    ret = connect(
        socket_fd, (const struct sockaddr *) &sa, sizeof(struct sockaddr_un)
    );
    if (ret == -1)
    {
        log_errno(LOG_ERROR, "Failed to connect to socket '%s': ", path);
        log_msg(LOG_INFO, "Check if server is running.");

        close(socket_fd);
        socket_fd = -1;
    }

    return socket_fd;
}

/* Transers ownership of result. Sets `nread` and `client_fd` */
char* read_incoming_message(int server_fd, size_t *nread, int *client_fd)
{
    (*nread) = 0;

    (*client_fd) = accept_client_connection(server_fd);
    if ((*client_fd) == -1) {
        return NULL;
    }

    char *msg = read_until_eof(*client_fd, nread);
    if (msg == NULL) {
        goto error;
    }

    if (msg[(*nread) - 1] != '\0')
    {
        log_msg(
            LOG_WARN, "Received message does not end with \\0. discarding..."
        );
        goto error;
    }

    return msg;

error:
    (*nread) = 0;
    free(msg);
    close(*client_fd);

    return NULL;
}

