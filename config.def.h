#ifndef CONFIG_H
#define CONFIG_H

#define DEF_LOG_LEVEL LOG_INFO

/* Path where tsup workdir will be created */
static const char TMPDIR[]         = "/tmp";

/* Name of the socket file */
static const char SOCKET_FILE[]    = "socket";

/* Server read buffer size  */
static const int  RECV_BUFFER_SIZE = 1024;

/* Size of server socket backlock */
static const int  SOCKET_BACKLOG   = 50;

/*
 * Number of second to wait for child processes to shutdown gracefully before
 * killing them
 */
static const int  KILL_TIMEOUT     = 1;

#endif /* CONFIG_H */
