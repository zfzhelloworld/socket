#define SOCK_FILE "/var/tmp/.sock_file"

#define REGIST 10
#define ATTACK 11

#define ATTACK_A 0
#define ATTACK_A_SCORE 10
#define ATTACK_B 1
#define ATTACK_B_SCORE 20

typedef struct sock_hdr_s
{
	int operation;
	int attack_type;
	char user_id[20];
	int life_score;
	int length;
} sock_hdr_t;
