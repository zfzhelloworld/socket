#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/streamsock.h"
#include "../include/socket_def.h"

static long timeout = 500; /*default socket timeout at 500 ms */

streamsock_t sock_g;

int init()
{
	streamsock_t *sock = &sock_g;
	if (new_local_svr_streamsock(sock, SOCK_FILE) != 0) {
		printf("new local svr streamsock error\n");
		return -1;
	}
	if (chmod(SOCK_FILE, 0777)) {
		printf("can't change socket permissions\n");
		return -2;
	}
	return 0;
}

void fini()
{
	delete_streamsock(sock_g);
	unlink(SOCK_FILE);
}

static void
process_request(streamsock_t sock, void *arg)
{
	data_t data;
	int rc;
	
	if ((rc = streamsock_set_timeout(sock, timeout, 1)) != 0) {
		printf("unable to set socket timeout. rc = %d\n", rc);
	}

	if ((rc = streamsock_set_timeout(sock, timeout, 0)) != 0) {
		printf("unable to set socket timeout. rc = %d\n", rc);
	}

	if (streamsock_read(sock, &data, sizeof(data)) == sizeof(data)) {
		printf("Get num from data: %d\n", data.num);
		printf("Get buff from data: %s\n", data.buff);
		
		rc = streamsock_write(sock, &data, sizeof(data));

		return;
	}

	delete_streamsock(sock);
	return; 
}

void wait_for_data()
{
	streamsock_accept(sock_g, process_request, NULL);
}

int main()
{
	
	if (init() == 0) {
		while (1) {
			(void) wait_for_data();
		}
		fini();
	}

	return 0;
}
