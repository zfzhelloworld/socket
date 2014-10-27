#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "../libs/streamsock.h"
#include "../include/socket_def.h"

void send_data_test(sock_hdr_t *hdr)
{
	streamsock_t sock;

	assert(hdr);
	if (new_local_streamsock(&sock, SOCK_FILE) == 0) {
		int rc = streamsock_write(sock, hdr, sizeof(*hdr));
		/*
		if (rc == sizeof(*data)) {
			(void) streamsock_shutdown(sock, SHUT_WR);
			if (streamsock_read(sock, &data_acc, sizeof(data_acc)) 
								== sizeof(data_acc)) {
				printf("Get num from data: %d\n", data_acc.num);
				printf("Get buff from data: %s\n", data_acc.buff);
			}
			
		}
		*/
	}	
}

void show_rule()
{
	printf("\n\n");
	printf("0 -> Create User\n");
	printf("A -> Tiny Attack (minus 10 point of life)\n");
	printf("B -> Midle Attack (minus 20 point of life)\n");
	printf("Q -> Quit\n");
	printf("Your cmd:");
}

int send_2_server(sock_hdr_t *sock_hdr)
{
	send_data_test(sock_hdr);

	return 0;
}

void client_run()
{
	sock_hdr_t sock_hdr;
	char user_id[20];
	char input_cmd;

	sock_hdr.life_score = 100;
	sock_hdr.operation = -1;
	sock_hdr.attack_type = -1;

	printf("Enter...\n");

	while (1) {
		show_rule();
		scanf("%c", &input_cmd);
		while(getchar() != '\n');
		//printf("User CMD: %c\n", input_cmd);
		switch(input_cmd) {
			case '0':
				printf("Input user id:");
				scanf("%s", &user_id);
				while(getchar() != '\n');
				strcpy(sock_hdr.user_id, user_id);
				sock_hdr.operation = REGIST;
				if (send_2_server(&sock_hdr) != 0) {
					printf("User: %s do Regist faild\n", user_id);
				}
				printf("\nCreate user: %s\n", user_id);
				break;
			case 'a':
			case 'A':
				sock_hdr.operation = ATTACK;
				sock_hdr.attack_type = ATTACK_A;
				if (send_2_server(&sock_hdr) != 0) {
					printf("User: %s do Attack A faild\n", user_id);
				}
				printf("User: %s tiny attack.\n", user_id);
				break;
			case 'b':
			case 'B':
				sock_hdr.operation = ATTACK;
				sock_hdr.attack_type = ATTACK_B;
				if (send_2_server(&sock_hdr) != 0) {
					printf("User: %s do Attack B faild\n", user_id);
				}
				printf("User: %s mid attack.\n", user_id);
				break;
			case 'q':
			case 'Q':
				printf("\nQuit,Bye!\n");
				return;
				break;
		}
	}
}

int main()
{
	client_run();		
	return 0;
}
