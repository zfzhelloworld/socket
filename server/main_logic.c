#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/streamsock.h"
#include "../include/socket_def.h"
#include "main_logic.h"

void init_play_room(play_room_t *room)
{
	(*room).regist_flag_a = 0;
	(*room).regist_flag_b = 0;
	(*room).is_full = 0;
}

int register_user(sock_hdr_t sock_hdr, play_room_t *room)
{
	if (room->is_full) {
		printf("error, play room is full...\n");
		return -1;
	}
	if (!room->regist_flag_a) {
		printf("Player A UserID: %s enter room...\n", sock_hdr.user_id);
		room->player_a.life_score = sock_hdr.life_score;
		strcpy(room->player_a.user_id, sock_hdr.user_id);
		room->regist_flag_a = 1;
	} else if (!room->regist_flag_b) {
		printf("Player B UserID: %s enter room...\n", sock_hdr.user_id);
		room->player_b.life_score = sock_hdr.life_score;
		strcpy(room->player_b.user_id, sock_hdr.user_id);
		room->regist_flag_b = 1;
		room->is_full = 1;
	}
	return 0;
}

int make_attack(sock_hdr_t hdr, player_t *player)
{
	switch (hdr.attack_type) {
		case ATTACK_A:
			if (player->life_score >= ATTACK_A_SCORE) {
				player->life_score -= ATTACK_A_SCORE;
			} else {
				player->life_score = 0;
			}
			break;
		case ATTACK_B:
			if (player->life_score >= ATTACK_B_SCORE) {
				player->life_score -= ATTACK_B_SCORE;
			} else {
				player->life_score = 0;
			}
			break;
	}
}

int handle_attack(sock_hdr_t sock_hdr, play_room_t *room)
{
	if (strcmp(sock_hdr.user_id, room->player_a.user_id) == 0) {
		//player A attack player B
		make_attack(sock_hdr, &(room->player_b));
	} else {
		make_attack(sock_hdr, &(room->player_a));
	}
}

void show_player_info(play_room_t room)
{
	player_t p_t;

	printf("------------------Player Info------------------\n");
	p_t = room.player_a;
	printf("Player A User: [%s] Life Score: [%d]\n", 
			p_t.user_id, p_t.life_score);
	p_t = room.player_b;
	printf("Player A User: [%s] Life Score: [%d]\n", 
			p_t.user_id, p_t.life_score);
	printf("---------------------End-----------------------\n");
}
