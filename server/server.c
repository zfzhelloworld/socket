#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/streamsock.h"
#include "../include/socket_def.h"
#include "main_logic.h"

static long timeout = 500; /*default socket timeout at 500 ms */

play_room_t play_room;
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
	
	init_play_room(&play_room);
	
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
	sock_hdr_t sock_hdr;
	int rc;

	
	if ((rc = streamsock_set_timeout(sock, timeout, 1)) != 0) {
		printf("unable to set socket timeout. rc = %d\n", rc);
	}

	if ((rc = streamsock_set_timeout(sock, timeout, 0)) != 0) {
		printf("unable to set socket timeout. rc = %d\n", rc);
	}

	if (streamsock_read(sock, &sock_hdr, sizeof(sock_hdr)) == sizeof(sock_hdr)) {
		
		printf("UserId: [%s], Operation: [%d] AttackType: [%d]\n", 
				sock_hdr.user_id, sock_hdr.operation, sock_hdr.attack_type);
		switch (sock_hdr.operation) {
			case REGIST:
				register_user(sock_hdr, &play_room);
				break;
			case ATTACK:
				handle_attack(sock_hdr, &play_room);
				show_player_info(play_room);
				break;
		}

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
