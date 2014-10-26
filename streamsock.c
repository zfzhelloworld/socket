/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 */


#include "streamsock.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

#define SSOCK_BACKLOG       20

#define SSOCK_IS_LOCAL      0x1

struct streamsock {
    int     fd;
    int     pipe[2];    /* used to snap out of accept & read */
    char    state;
};

typedef struct sockaddr SA;

/* -------------------------------------------------------------------------- */
#define VALIDATE_SOCK(s)    ((s) != NULL && (s)->fd >= 0)


static void
init_streamsock(void)
{
    static int init = 0;

    if (init == 0) {
        /* ignore SIGPIPE and check for EPIPE directly */
        signal(SIGPIPE, SIG_IGN);
        init = 1;
    }
}

/*
 * If the argument fd > 0, a valid descriptor is used to construct the new
 * socket struct instead of calling socket.
 */
static int
init_sock(streamsock_t *s, int domain, int is_svr, int fd)
{
    if ((*s = (streamsock_t)malloc(sizeof (struct streamsock))) != NULL) {
        memset(*s, 0x00, sizeof (**s));
        (**s).fd = (fd < 0 ? socket(domain, SOCK_STREAM, 0) : fd);
        if ((**s).fd >= 0) {
            if (is_svr) {
                if (pipe((**s).pipe) == 0) {
                    int fd = (**s).pipe[0];
                    (void) fcntl(fd, F_SETFL, O_NONBLOCK);
                    return (0);
                }
            } else {
                (**s).pipe[0] = (**s).pipe[1] = -1;
                return (0);
            }
        }
        free(*s);
    }

    return (-1);
}

static int
init_local_addr_sock(streamsock_t *s, struct sockaddr_un *addr,
                     const char *svr_path, int is_svr)
{
    assert(s && addr && svr_path);

    memset(addr, 0x00, sizeof (*addr));
    addr->sun_family = AF_LOCAL;
    strncpy(addr->sun_path, svr_path, sizeof (addr->sun_path) - 1);

    return (init_sock(s, AF_LOCAL, is_svr, -1));
}

static int
init_inet_addr_sock(streamsock_t *s, struct sockaddr_in *addr,
                    in_addr_t svr_addr, in_port_t port, int is_svr)
{
    assert(s && addr);

    memset(addr, 0x00, sizeof (*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(svr_addr);
    addr->sin_port = htons(port);

    return (init_sock(s, AF_INET, is_svr, -1));
}

/* -------------------------------------------------------------------------- */
int
new_local_streamsock(streamsock_t *sock, const char *svr_path)
{
    struct sockaddr_un  addr;
    int rc;

    if (sock == NULL || svr_path == NULL) {
        return (-1);
    }
    init_streamsock();

    if ((rc = init_local_addr_sock(sock, &addr, svr_path, 0)) == 0) {
        if ((rc = connect((**sock).fd, (SA*)&addr,
                          SUN_LEN(&addr))) == 0) {
            (**sock).state = SSOCK_CONNECTED;
            return (0);
        }
        delete_streamsock(*sock);
    }

    return (rc);
}

int
new_local_svr_streamsock(streamsock_t *sock, const char *svr_path)
{
    struct sockaddr_un  addr;
    int rc;

    if (sock == NULL || svr_path == NULL) {
        return (-1);
    }
    init_streamsock();
    if ((rc = init_local_addr_sock(sock, &addr, svr_path, 1)) == 0) {
        /* Must unlink any existing file or bind will fail */
        unlink(svr_path);
        if ((rc = bind((**sock).fd, (SA*)&addr, SUN_LEN(&addr))) == 0) {
            rc = listen((**sock).fd, SSOCK_BACKLOG);
            assert(rc == 0);
            (**sock).state = SSOCK_LISTENING;
            return (0);
        }
        delete_streamsock(*sock);
    }

    return (rc);
}

int
new_streamsock(streamsock_t *sock, in_addr_t svr_addr, in_port_t port)
{
    struct sockaddr_in  addr;
    int rc;

    init_streamsock();
    if ((rc = init_inet_addr_sock(sock, &addr, svr_addr, port, 0)) == 0) {
        if ((rc = connect((**sock).fd, (SA*)&addr,
                          sizeof (addr))) == 0) {
            (**sock).state = SSOCK_CONNECTED;
            return (0);
        }
        delete_streamsock(*sock);
    }

    return (rc);
}

int
new_svr_streamsock(streamsock_t *sock, in_addr_t svr_addr, in_port_t port)
{
    struct sockaddr_in  addr;
    int rc, on = 1;

    init_streamsock();
    if ((rc = init_inet_addr_sock(sock, &addr, svr_addr, port, 1)) == 0) {
        /* turn on the reuse address option, don't care if it fails */
        (void) setsockopt((**sock).fd, SOL_SOCKET, SO_REUSEADDR, &on,
                          sizeof (on));
        rc = bind((**sock).fd, (SA*)&addr, sizeof (addr));
        if (rc == 0) {
            rc = listen((**sock).fd, SSOCK_BACKLOG);
            assert(rc == 0);
            (**sock).state = SSOCK_LISTENING;
            return (0);
        }
        delete_streamsock(*sock);
    }

    return (rc);
}

/* -------------------------------------------------------------------------- */
void
delete_streamsock(streamsock_t sock)
{
    if (VALIDATE_SOCK(sock)) {
        close(sock->fd);
        if (sock->pipe[0] != -1) {
            close(sock->pipe[0]);
        }
        if (sock->pipe[1] != -1) {
            close(sock->pipe[1]);
        }
        free(sock);
    }
}

int
streamsock_shutdown(streamsock_t sock, int how)
{
    if (VALIDATE_SOCK(sock)) {
        shutdown(sock->fd, how);
        switch (how) {
            case SHUT_RD:
                sock->state = SSOCK_RD_SHUTDOWN;
                break;
            case SHUT_WR:
                sock->state = SSOCK_WR_SHUTDOWN;
                break;
            case SHUT_RDWR:
                sock->state = SSOCK_CLOSED;
                break;
        }
        return (0);
    }

    return (-1);
}

/* -------------------------------------------------------------------------- */
int
streamsock_family(streamsock_t sock, int *family)
{
    int rc = -1;
    assert(family);

    if (VALIDATE_SOCK(sock)) {
        struct sockaddr_un  addr;
        socklen_t   len = sizeof (addr);
        rc = getsockname(sock->fd, (SA*)&addr, &len);
        if (rc == 0) {
            if (len) {
                *family = ((SA*)&addr)->sa_family;
            } else {
                *family = AF_LOCAL;
            }
        }
    }

    return (rc);
}

int
streamsock_state(streamsock_t sock, ssock_state_t *state)
{
    int rc = -1;
    assert(state);

    if (VALIDATE_SOCK(sock)) {
        *state = sock->state;
        rc = 0;
    }

    return (rc);
}

int
streamsock_fileno(streamsock_t sock)
{
    return (VALIDATE_SOCK(sock) ? sock->fd : -1);
}

int
streamsock_set_timeout(streamsock_t sock, long millisec, int is_write)
{
    struct timeval to;
    int rc = -1, opt;

    if (VALIDATE_SOCK(sock)) {
        to.tv_sec = millisec / 1000;
        to.tv_usec = (millisec % 1000) * 1000;
        opt = is_write ? SO_SNDTIMEO : SO_RCVTIMEO;
        rc = setsockopt(sock->fd, SOL_SOCKET, opt, &to, sizeof (to));
    }

    return (rc);
}

/* -------------------------------------------------------------------------- */
static int
streamsock_do_accept(streamsock_t sock, fd_set *fds,
                      handle_connect_t handler, void *arg)
{
    /* local address is the biggest */
    struct sockaddr_un cliaddr;
    socklen_t addrlen = sizeof (cliaddr);
    int fd, rc = 0;

    fd = accept(sock->fd, (SA*)&cliaddr, &addrlen);
    assert(fd >= 0);
    if (fd >= 0) {
        streamsock_t ns;
        int family;

        rc = streamsock_family(sock, &family);
        assert(rc == 0);
        rc = init_sock(&ns, family, 0, fd);
        if (rc == 0) {
            ns->state = SSOCK_CONNECTED;
            /* transfer ownership of ns */
            handler(ns, arg);
        }
    }
    if (FD_ISSET(sock->pipe[0], fds)) {
        /* clear the pipe and snap out of it */
        char c;
        while (read(sock->pipe[0], &c, sizeof (c)) > 0)
            ;

        rc = 1;
    }

    return rc;
}

static int
sockets_valid(streamsock_t socks[], int num_socks)
{
    int i;

    for (i = 0; i < num_socks; i++) {
        if (!VALIDATE_SOCK(socks[i]) || ((socks[i])->state != SSOCK_LISTENING))
            return 0;
    }

    return 1;
}

int
streamsocks_accept(streamsock_t socks[], int num_socks,
    handle_connect_t handler, void *arg)
{
    int i, rc;

    assert(handler != NULL);

    if (sockets_valid(socks, num_socks)) {
        fd_set fds;
        int nfd = 0;

        FD_ZERO(&fds);

        for (i = 0; i < num_socks; i++)
            assert((socks[i])->pipe[0] >= 0 && (socks[i])->pipe[1] >= 0);

        for (i = 0; i < num_socks; i++) {
            if (i == 0)
                nfd = ((socks[i])->fd > (socks[i])->pipe[0]) ? (socks[i])->fd : (socks[i])->pipe[0];
            else {
                nfd = ((socks[i])->fd > nfd) ? (socks[i])->fd : nfd;
                nfd = ((socks[i])->pipe[0] > nfd) ? (socks[i])->pipe[0] : nfd;
            }
        }

        nfd += 1;

        for (i = 0; i < num_socks; i++)
            (socks[i])->state = SSOCK_ACCEPTING;

        for (;;) {
            for (i = 0; i < num_socks; i++) {
                FD_SET((socks[i])->fd, &fds);
                FD_SET((socks[i])->pipe[0], &fds);
            }

            rc = select(nfd, &fds, NULL, NULL, NULL);
            if (rc < 0) {
                if (errno == EINTR) {
                    continue;
                } else {
                    break;
                }
            }

            if (rc > 0) {
                for (i = 0; i < num_socks; i++) {
                    if (FD_ISSET((socks[i])->fd, &fds))
                        rc = streamsock_do_accept(socks[i], &fds, handler, arg);
                }

                if (rc == 1)
                    break;
            }
        }

        /* set them back to listening */
        for (i = 0; i < num_socks; i++)
            (socks[i])->state = SSOCK_LISTENING;
    }
    else {
        return -1;
    }

    return (0);
}

int
streamsock_accept(streamsock_t sock, handle_connect_t handler, void *arg)
{
    int rc;
    streamsock_t socks[1];

    socks[0] = sock;
    rc = streamsocks_accept(socks, 1, handler, arg);

    return rc;
}

int
streamsock_stop_accept(streamsock_t sock)
{
    if (VALIDATE_SOCK(sock) && sock->pipe[1] >= 0) {
        char c = 'a';
        int rc = write(sock->pipe[1], &c, sizeof (c));
        return (rc);
    }

    return (-1);
}

/* -------------------------------------------------------------------------- */
ssize_t
streamsock_read(streamsock_t sock, void *buf, size_t length)
{
    int rc = -1;

    if (VALIDATE_SOCK(sock)) {
        size_t bytes_left = length;

        while (bytes_left > 0) {
            ssize_t n = read(sock->fd, buf, bytes_left);
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                } else {
                    return (-1);
                }
            }
            if (n == 0) {
                if (sock->state == SSOCK_CONNECTED ||
                    sock->state == SSOCK_WR_SHUTDOWN) {
                    sock->state = SSOCK_CLOSED;
                }
                break;
            }
            bytes_left -= n;
            buf += n;
        }
        rc = length - bytes_left;
    }

    return (rc);
}

ssize_t
streamsock_write(streamsock_t sock, const void *buf, size_t length)
{
    int rc = -1;

    if (VALIDATE_SOCK(sock)) {
        size_t  bytes_left = length;
        while (bytes_left > 0) {
            ssize_t n = write(sock->fd, buf, bytes_left);
            if (n <= 0) {
                if (errno == EINTR) {
                    continue;
                } else if (errno == EPIPE) {
                    if (sock->state == SSOCK_CONNECTED ||
                        sock->state == SSOCK_RD_SHUTDOWN) {
                        sock->state = SSOCK_CLOSED;
                    }
                    return (-1);
                } else if (errno) {
                    return (-1);
                }
            }
            bytes_left -= n;
            buf += n;
        }
        rc = length - bytes_left;
    }

    return (rc);
}
