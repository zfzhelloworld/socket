#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "streamsock.h"
#include "socket_def.h"

void send_data_test(data_t *data)
{
	streamsock_t sock;
	data_t data_acc;

	assert(data);
	if (new_local_streamsock(&sock, SOCK_FILE) == 0) {
		int rc = streamsock_write(sock, data, sizeof(*data));
		if (rc == sizeof(*data)) {
			(void) streamsock_shutdown(sock, SHUT_WR);
			if (streamsock_read(sock, &data_acc, sizeof(data_acc)) 
								== sizeof(data_acc)) {
				printf("Get num from data: %d\n", data_acc.num);
				printf("Get buff from data: %s\n", data_acc.buff);
			}
			
		}
	}	
}

int main(int argc, char *argv[])
{
	data_t data;
	
	data.num = 888;
	strcpy(data.buff, argv[1]);

	send_data_test(&data);

	return 0;
}
