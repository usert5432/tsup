#ifndef SOCKET_HANDLERS_H
#define SOCKET_HANDLERS_H

#include <stddef.h>

struct sockaddr_un;

/* Returns -1 on error */
int accept_client_connection(int server_fd);

void close_socket(int socket_fd);

/* Returns socket fd and sets `sa` on success. Returns -1 on error */
int setup_unix_socket(const char *path, struct sockaddr_un *sa);

/* Returns socket fd on success. Returns -1 on error */
int open_client_socket(const char *path);
/* Returns socket fd on success. Returns -1 on error */
int open_server_socket(const char *path);

/* Transers ownership of result. Sets `nread` and `client_fd` */
char* read_incoming_message(int server_fd, size_t *nread, int *client_fd);

#endif /* SOCKET_HANDLERS_H */
