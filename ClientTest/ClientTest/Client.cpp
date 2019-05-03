// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  
#define INITGUID
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <windows.h>   // include important windows stuff
#include <iostream>
#include <thread>

using namespace std;

#pragma comment (lib, "ws2_32.lib")

#include "Protocol.h"

int retval;
SOCKET sock;
char buf[MAX_PACKET_SIZE];

void do_recv()
{
	//while (true)
	//{
	//	retval = recv(sock, buf, MAX_PACKET_SIZE, 0);

	//	if (retval == 0 || retval == SOCKET_ERROR)
	//	{
	//		cout << "접속 종료" << endl;
	//		closesocket(sock);
	//		break;
	//	}

	//	while (retval > 0)
	//	{
	//		BYTE type = buf[0];

	//		if (type == TYPE_POSITION) {
	//			memset(&pos_packet, 0, sizeof(t_Position));
	//			memcpy(&pos_packet, buf, sizeof(t_Position));
	//		
	//			cout << "Position Data Length : " << retval << endl;
	//			cout << "Recv Data >> " << "id : " << int(pos_packet.id) << ", x : " << pos_packet.x << ",y : " << pos_packet.y << endl;
	//			if (myIndex == INIT_ID)
	//				myIndex = pos_packet.id;
	//		
	//			if (m_Objects[int(pos_packet.id)] == NULL)
	//				AddObject(int(pos_packet.id), pos_packet.x, pos_packet.y, 0.f, 1.f, 1.f, 1.f);
	//			else
	//				m_Objects[int(pos_packet.id)]->setPosition(pos_packet.x, pos_packet.y, 0.f);
	//		
	//			// 여러 Client Position 정보가 버퍼에 누적되어있을 수도 있으니 땡겨주자.
	//			memmove(buf, buf + sizeof(t_Position), retval - sizeof(t_Position));
	//			retval -= sizeof(t_Position);
	//		}
	//		
	//		if (type == TYPE_DISCONNECT) {
	//			memset(&dis_packet, 0, sizeof(t_Disconnect));
	//			memcpy(&dis_packet, buf, sizeof(t_Disconnect));
	//		
	//			cout << "DISCONNECT Data Length : " << retval << endl;
	//			cout << "Recv Data >> " << "id : " << int(dis_packet.id) << endl;
	//		
	//			DeleteObject(int(dis_packet.id));
	//		
	//			// 여러 Client Position 정보가 버퍼에 누적되어있을 수도 있으니 땡겨주자.
	//			memmove(buf, buf + sizeof(t_Disconnect), retval - sizeof(t_Disconnect));
	//			retval -= sizeof(t_Disconnect);
	//		}

	//		cout << "================================================================================" << std::endl;
	//	}

	//	memset(buf, 0, sizeof(MAX_PACKET_SIZE));
	//}
}

void do_send()
{
	while (true)
	{
		int key = 1;

		switch (key) {
		case CS_CONNECT:
			cs_packet_connect p_connect;
			p_connect.size = sizeof(p_connect);
			p_connect.type = CS_CONNECT;

			retval = send(sock, (char *)&p_connect, sizeof(cs_packet_connect), 0);
			break;
		}

		Sleep(3000);
	}
}

int main() {

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket
	sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET)
	{
		cout << "Error : WSASocket()\n";
		return 1;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	//serveraddr.sin_addr.s_addr = inet_addr(serverip);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		cout << "Error : Connect()\n";
		return 1;
	}
	
	cout << "connect complete" << endl;

	//thread t1(do_recv);
	thread t2{ do_send };
	//
	////t1.join();
	t2.join();


	WSACleanup();
	return 0;
}