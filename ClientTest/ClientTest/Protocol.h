constexpr int SERVER_PORT = 7000;
constexpr int MAX_USER = 10;
constexpr int MAX_PACKET_SIZE = 1024;

constexpr int CS_UP = 1;
constexpr int CS_DOWN = 2;
constexpr int CS_LEFT = 3;
constexpr int CS_RIGHT = 4;
		  
constexpr int SC_LOGIN = 1;
constexpr int SC_POS = 2;
constexpr int SC_PUT_PLAYER = 3;
constexpr int SC_REMOVE = 4;

#pragma pack(push, 1)

struct cs_packet_up {
	char size;
	char type;
};

struct cs_packet_down {
	char size;
	char type;
};

struct cs_packet_left {
	char size;
	char type;
};

struct cs_packet_right {
	char size;
	char type;
};

struct sc_packet_login {
	char size;
	char type;
	char id;
};

struct sc_packet_pos {
	char size;
	char type;
	char id;
	char x, y;
};

struct sc_packet_put_player {
	char size;
	char type;
	char id;
	char x, y;
};

struct sc_packet_remove {
	char size;
	char type;
	char id;
};

#pragma pack (pop)