#define WIN32_LEAN_AND_MEAN  
#define INITGUID
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <map>
#include <vector>
#include <thread>

using namespace std;

#include <winsock2.h>
#include <windows.h>
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024

struct OVER_EX {
	WSAOVERLAPPED overlapped;
	WSABUF dataBuffer;
	char messageBuffer[MAX_BUFFER];
	bool is_recv;
	OVER_EX() {
		ZeroMemory(&overlapped, sizeof(overlapped));
		ZeroMemory(messageBuffer, sizeof(messageBuffer));
		dataBuffer.buf = messageBuffer;
		dataBuffer.len = sizeof(messageBuffer);
		is_recv = true;
	}
};

class SOCKETINFO
{
public:
	OVER_EX over_ex;
	SOCKET socket;
	char packetBuffer[MAX_BUFFER];
	int prev_size;
	float x, y, z;
	float lx, ly, lz;
	bool connected;

	SOCKETINFO() : over_ex{} {
		ZeroMemory(&over_ex.overlapped, sizeof(WSAOVERLAPPED));
		x = y = z = 0;
		lx = ly = lz = 0;
		prev_size = 0;
		connected = false;
	}

	SOCKETINFO(SOCKET s) : over_ex{} {
		socket = s;
		ZeroMemory(&over_ex.overlapped, sizeof(WSAOVERLAPPED));
		x = y = z = 0;
		lx = ly = lz = 0;
		prev_size = 0;
		connected = false;
	}

	~SOCKETINFO() {
	}

	SOCKETINFO& operator=(SOCKETINFO&& other) {
		this->socket = move(other.socket);
		this->over_ex = move(other.over_ex);
		this->over_ex.dataBuffer.buf = this->over_ex.messageBuffer;
		this->over_ex.dataBuffer.len = sizeof(this->over_ex.messageBuffer);
		memmove(this->packetBuffer, other.packetBuffer, MAX_BUFFER);
		this->prev_size = move(other.prev_size);
		this->x = move(other.x);
		this->y = move(other.y);
		this->z = move(other.z);
		this->lx = move(other.lx);
		this->ly = move(other.ly);
		this->lz = move(other.lz);
		this->connected = move(other.connected);
		return *this;
	}

	SOCKETINFO(SOCKETINFO&& other) {

	}
};

HANDLE g_iocp;
map<char, SOCKETINFO> clients;

void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"  에러" << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	while (true);
}

void ErrorDisplay(const char * location)
{
	error_display(location, WSAGetLastError());
}


void do_recv(char id)
{
	DWORD flags = 0;

	ZeroMemory(&clients[id].over_ex.overlapped, sizeof(WSAOVERLAPPED));

	int retval = WSARecv(clients[id].socket, &clients[id].over_ex.dataBuffer, 1,
		NULL, &flags, &clients[id].over_ex.overlapped, 0);

	if (retval != 0)
		// NumOfByteRecv => NULL : 받지 않겠다는 강력한 표시
	{
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) error_display("recv Error! ", err_no);
	}
}


void sendPacket(char key, void *ptr)
{
	char *packet = reinterpret_cast<char *>(ptr);
	OVER_EX *over = new OVER_EX;
	over->is_recv = false;
	memcpy(over->messageBuffer, packet, packet[0]);
	over->dataBuffer.buf = over->messageBuffer;
	over->dataBuffer.len = over->messageBuffer[0];

	//cout << int(over->messageBuffer[0]) << ", " << int(over->messageBuffer[1]) << endl;

	ZeroMemory(&over->overlapped, sizeof(WSAOVERLAPPED));
	int res = WSASend(clients[key].socket, &over->dataBuffer, 1, NULL, 0,
		&over->overlapped, NULL);
	if (0 != res) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) error_display("Send Error! ", err_no);
	}
}

void process_packet(char key, char *buffer)
{
	switch (buffer[1]) {
	case CS_CONNECT:
		cout << "[" << int(key) << "] Clients Login\n";
		sc_packet_login p_login;
		p_login.id = key;
		p_login.x = 0.f;
		p_login.y = 0.f;
		p_login.z = 0.f;
		p_login.lx = 0.f;
		p_login.ly = 0.f;
		p_login.lz = 1.f;
		p_login.type = SC_LOGIN;
		p_login.size = sizeof(sc_packet_login);

		sc_packet_put_player p_put;
		p_put.id = key;
		p_put.x = 0.f;
		p_put.y = 0.f;
		p_put.z = 0.f;
		p_put.lx = 0.f;
		p_put.ly = 0.f;
		p_put.lz = 1.f;
		p_put.type = SC_PUT_PLAYER;
		p_put.size = sizeof(sc_packet_put_player);

		clients[key].x = 0.f;
		clients[key].y = 0.f;
		clients[key].z = 0.f;
		clients[key].lx = 0.f;
		clients[key].ly = 0.f;
		clients[key].lz = 1.f;
		clients[key].connected = true;

		// 자신(key)과 다른 클라에게 자기 위치 정보 Send
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (clients[i].connected)
			{
				if (i == key)
					sendPacket(key, &p_login);
				else
					sendPacket(i, &p_put);
			}
		}
		// 로그인 시, 접속 중인 다른 클라 위치 정보를 자신에게 Send
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (clients[i].connected)
			{
				if (i != key) 
				{
					p_put.id = i;
					p_put.x = clients[i].x;
					p_put.y = clients[i].y;
					p_put.z = clients[i].z;
					p_put.lx = clients[i].lx;
					p_put.ly = clients[i].ly;
					p_put.lz = clients[i].lz;
					p_put.type = SC_PUT_PLAYER;
					p_put.size = sizeof(sc_packet_put_player);
					sendPacket(key, &p_put);
				}
			}
		}
		break;

	case CS_DISCONNECT:
		cout << "[" << int(key) << "] Clients Disconnect\n";
		break;

	case CS_MOVE:
		cout << "[" << int(key) << "] Clients Move\n";
		cs_packet_move *p_move;
		p_move = reinterpret_cast<cs_packet_move*>(buffer);
		//cout << "0x" << hex << p_move->flag << endl;

		float distance = 1.f;

		if (p_move->flag & 0x01) {  // forward
			clients[key].lz = -1.f;
			clients[key].z += distance;
		}
		if (p_move->flag & 0x02) {	// backward
			clients[key].lz = 1.f;
			clients[key].z -= distance;
		}
		if (p_move->flag & 0x04) {	// left
			clients[key].lx = 1.f;
			clients[key].x -= distance;
		}
		if (p_move->flag & 0x08) {	// right
			clients[key].lx = -1.f;
			clients[key].x += distance;
		}

		sc_packet_pos p_pos;
		p_pos.id = key;
		p_pos.x = clients[key].x;
		p_pos.y = clients[key].y;
		p_pos.z = clients[key].z;
		p_pos.lx = clients[key].lx;
		p_pos.ly = clients[key].ly;
		p_pos.lz = clients[key].lz;
		p_pos.type = SC_POS;
		p_pos.size = sizeof(sc_packet_pos);
		
		// Move한 정보를 브로드캐스팅
		for (int i = 0; i < MAX_USER; ++i)
		{
			if(clients[i].connected)
				sendPacket(i, &p_pos);
		}
		break;
	}
}

void worker_thread()
{
	while (true)
	{
		DWORD io_byte;
		ULONG io_key;
		OVER_EX *over_ex;

		BOOL is_error = GetQueuedCompletionStatus(g_iocp, &io_byte, (PULONG_PTR)&io_key, reinterpret_cast<LPWSAOVERLAPPED *>(&over_ex), INFINITE);

		char key = static_cast<char>(io_key);

		if (io_byte == 0)
		{
			cout << "[" << int(key) << "] Clients Disconnect\n";
			sc_packet_remove p_remove;
			p_remove.id = key;
			p_remove.type = SC_REMOVE;
			p_remove.size = sizeof(sc_packet_remove);

			for (int i = 0; i < MAX_USER; ++i)
			{
				if (clients[i].connected)
					if (i != key)
						sendPacket(i, &p_remove);
					else
						clients[i].connected = false;
			}	
		}

		if (true == over_ex->is_recv) {
			// 패킷 조립
			int rest = io_byte;
			char *wptr = over_ex->messageBuffer;

			int packet_size = 0;

			if (0 < clients[key].prev_size) {
				packet_size = int(clients[key].packetBuffer[0]);
			}

			while (0 < rest) {
				if (0 == packet_size)
					packet_size = wptr[0];

				int required = packet_size - clients[key].prev_size;

				if (required <= rest) {
					memcpy(clients[key].packetBuffer + clients[key].prev_size, wptr, required);
					process_packet(key, clients[key].packetBuffer);
					rest -= required;
					wptr += required;
					packet_size = 0;
					clients[key].prev_size = 0;
				}
				else {
					memcpy(clients[key].packetBuffer + clients[key].prev_size,
						wptr, rest);
					rest = 0;
					clients[key].prev_size += rest;
				}
			}
			do_recv(key);
		}
		else {
			//cout << "mmmmmmmmmmmmmmm\n";
			delete over_ex;
		}
	}
}

void do_accept()
{
	// 1. 소켓생성  
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	// 마지막 인자로 비동기식이라는 것을 알려주는 WSA_FLAG_OVERLAPPED
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error - Invalid socket\n";
		return;
	}

	// 서버정보 객체설정
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 2. 소켓설정
	if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		printf("Error - Fail bind\n");
		// 6. 소켓종료
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

	// 3. 수신대기열생성
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Error - Fail listen\n");
		// 6. 소켓종료
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

	
	while (1)
	{
		SOCKADDR_IN clientAddr;
		ZeroMemory(&clientAddr, sizeof(SOCKADDR_IN));
		int addrLen = sizeof(SOCKADDR_IN);
		DWORD flags;

		SOCKET clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		// listenSocket이 비동기여야 clientSocket도 비동기소켓으로 된다
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "Error - Accept Failure\n";
			return;
		}

		int new_id = -1;
		for (int i = 0; i < MAX_USER; ++i) {
			if (!clients[i].connected) {
				new_id = i;
				break;
			}
		}

		if (-1 == new_id) {
			cout << "MAX USER overflow\n";
			continue;
		}

		SOCKETINFO socketInfo(clientSocket);

		clients[new_id] = std::move(socketInfo);
		flags = 0;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, new_id, 0);

		do_recv(new_id);
	}

	// 6-2. 리슨 소켓종료
	closesocket(listenSocket);

	// Winsock End
	WSACleanup();

	return;
}

int main()
{
	vector<thread> worker_threads;
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	// Winsock Start - windock.dll 로드
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "Error - Can not load 'winsock.dll' file\n";
		return 0;
	}

	for (int i = 0; i < 4; ++i)
		worker_threads.push_back(thread{ worker_thread });

	thread accept_thread{ do_accept };

	for (auto &th : worker_threads) th.join();
	accept_thread.join();
}