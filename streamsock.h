/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	STREAMSOCK_H_
#define	STREAMSOCK_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
typedef enum {
	SSOCK_INVALID = -1,
	SSOCK_CONNECTED = 0,
	SSOCK_LISTENING = 2,
	SSOCK_ACCEPTING = 3,
	SSOCK_CLOSED = 4,
	SSOCK_RD_SHUTDOWN = 5,
	SSOCK_WR_SHUTDOWN = 6
} ssock_state_t;

typedef struct streamsock	*streamsock_t;

typedef void (*handle_connect_t)(streamsock_t new_sock, void *arg);

/* -------------------------------------------------------------------------- */
 
/*
 * Construct a new active local streamsock_t instance that connects to a server
 * socket listening to svr_path. svr_path must be an absolute path.
 */
int new_local_streamsock(streamsock_t *sock, const char *svr_path);

/*
 * Construct a new passive local streamsock_t instance bound to svr_path.
 * svr_path must be an absolute path.
 */
int new_local_svr_streamsock(streamsock_t *sock, const char *svr_path);

/*
 * Construct a new active TCP streamsock_t instance that connects to a server
 * socket listening to IP address svr_addr and port.
 */
int new_streamsock(streamsock_t *sock, in_addr_t svr_addr, in_port_t port);

/*
 * Construct a new passive TCP streamsock_t instance listening to IP address
 * svr_addr and port. INADDR_ANY can be passed to svr_addr as wildcard value.
 */
int new_svr_streamsock(streamsock_t *sock, in_addr_t svr_addr, in_port_t port);

/*
 * Destroy a streamsock_t. This function will not terminate the connection if
 * the reference count for the socket is non-zero after the socket is closed,
 * e.g., the descriptor is shared by a child.
 */
void delete_streamsock(streamsock_t sock);

/*
 * Terminate the connection associated with the socket. The argument how can 
 * be:
 * SHUTDOWN_RD: the read side of the connection is terminated.
 * SHUTDOWN_WR: the write side of the connection is terminated.
 * SHUTDOWN_RDWR: both the read and write side of the connection is terminated.
 *
 * Note that the connection is terminated regardless of the reference count on
 * the socket descriptor.
 */
int streamsock_shutdown(streamsock_t sock, int how);

/* Returns protocol family in <family> */ 
int streamsock_family(streamsock_t sock, int *family);

/* Returns the current state of the socket */
int streamsock_state(streamsock_t sock, ssock_state_t *state);

/* Returns the raw descriptor of the socket */
int streamsock_fileno(streamsock_t sock);

/*
 * Set the read/write timeout for a streamsock. Timeout is in millisecond.
 * Timeout of 0 means no timeout (block forever). To set read timeout pass 0
 * in is_write. To set write timeout pass 1 in is_write.
 */
int streamsock_set_timeout(streamsock_t sock, long millisec, int is_write);

/*
 * To block and wait for connections to come in. The ownership of the newly 
 * connected socket will be transferred to the callback <handler>.
 */
int streamsock_accept(streamsock_t sock, handle_connect_t handler, void *arg);

/*
 * Construct a new passive TCP streamsock_t instance listening to multiple
 * socket connections. All of the sockets are expected to take the same
 * action, i.e. connect handler, with the same arg.
 */
int streamsocks_accept(streamsock_t socks[], int num_socks,
    handle_connect_t handler, void *arg);

/*
 * This function is used to unblock accept. It is safe to be called from another
 * thread, but does not guard against race condition when called from signal 
 * handlers, i.e., may lose signals.
 */
int streamsock_stop_accept(streamsock_t sock);

/*
 * Reading from a socket with the read side closed returns 0. 
 * Return value smaller than length indicates the number of bytes read before
 * the connection was terminated.
 */
ssize_t streamsock_read(streamsock_t sock, void *buf, size_t length);

/*
 * Return value of -1 indicates that the write failed. It does not distinguish
 * case where a partial write occurred (unlike read). This is because even if
 * the partial write is transmitted, there is no guarantee that the peer
 * application received it.
 */
ssize_t streamsock_write(streamsock_t sock, const void *buf, size_t length);

/* perhaps need a readv/writev version? */

#ifdef __cplusplus
}
#endif

#endif	/* STREAMSOCK_H_ */
