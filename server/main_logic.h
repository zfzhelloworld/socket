typedef struct player
{
	char user_id[20];
	int life_score;
} player_t;

typedef struct play_room
{
	int regist_flag_a;
	int regist_flag_b;
	int is_full;
	player_t player_a;
	player_t player_b;
} play_room_t;
