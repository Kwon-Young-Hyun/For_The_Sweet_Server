#define WIN32_LEAN_AND_MEAN  
#define INITGUID
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "header.h"
#include "Protocol.h"
#include "Player.h"
#include "Physx.h"
#include "Timer.h"
#include "Util.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_BUFFER 1024

PxVec3 PlayerInitPosition[8] = {
	PxVec3(0, 10.1, -190), PxVec3(50, 10.1, -190), PxVec3(-50, 10.1, -190), PxVec3(100, 10.1, -190), PxVec3(-100, 10.1, -190),
	PxVec3(150, 10.1, -190), PxVec3(-150, 10.1, -190), PxVec3(200, 10.1, -190)
};

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
	CPlayer *playerinfo;
	//float vx, vy, vz;
	bool connected;

	SOCKETINFO() : over_ex{} {
		ZeroMemory(&over_ex.overlapped, sizeof(WSAOVERLAPPED));
		playerinfo = new CPlayer();
		prev_size = 0;
		connected = false;
	}

	SOCKETINFO(SOCKET s) : over_ex{} {
		socket = s;
		ZeroMemory(&over_ex.overlapped, sizeof(WSAOVERLAPPED));
		playerinfo = new CPlayer();
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
		this->playerinfo = other.playerinfo;
		return *this;
	}

	SOCKETINFO(SOCKETINFO&& other) {

	}
};

HANDLE g_iocp;
map<char, SOCKETINFO> clients;

CPhysx *gPhysx;

vector<PxVec3> gMapVertex;
vector<int> gMapIndex;
vector<pair<int, float>> aniInfo;

CGameTimer gGameTimer;
volatile bool start = false;

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
	std::wcout << L"  ����" << lpMsgBuf << std::endl;
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

	if (0 != retval) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
		{
			cout << "Error1 - IO pending Failure\n";
		}
	}
	else {
		//cout << "Non Overlapped Recv~~~~~~~~~~~~.\n";
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
		if (WSA_IO_PENDING != err_no)
		{
			cout << "Error1 - IO pending Failure\n";
		}
	}
	else {
		//cout << "Non Overlapped Send~~~~~~~~~~~~.\n";
	}
}

void process_packet(char key, char *buffer)
{
	switch (buffer[1]) {
	case CS_CONNECT:
		//cout << "[" << int(key) << "] Clients Login\n";
		sc_packet_login p_login;
		p_login.id = key;
		p_login.x = PlayerInitPosition[key].x;
		p_login.y = PlayerInitPosition[key].y;
		p_login.z = PlayerInitPosition[key].z;
		p_login.vx = 0.f;
		p_login.vy = 0.f;
		p_login.vz = 1.f;
		p_login.type = SC_LOGIN;
		p_login.size = sizeof(sc_packet_login);

		sc_packet_put_player p_put;
		p_put.id = key;
		p_put.x = PlayerInitPosition[key].x;
		p_put.y = PlayerInitPosition[key].y;
		p_put.z = PlayerInitPosition[key].z;
		p_put.vx = 0.f;
		p_put.vy = 0.f;
		p_put.vz = 1.f;
		p_put.type = SC_PUT_PLAYER;
		p_put.size = sizeof(sc_packet_put_player);

		clients[key].playerinfo->setPosition(PlayerInitPosition[key]);
		clients[key].playerinfo->setVelocity(PxVec3(0, 0, 1));
		clients[key].playerinfo->setPlayerController(gPhysx);
		clients[key].connected = true;

		// �ڽ�(key)�� �ٸ� Ŭ�󿡰� �ڱ� ��ġ ���� Send
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (clients[i].connected)
			{
				if (i == key)
				{
					cout << "Login Packet Send\n";
					sendPacket(i, &p_login);
				}
				else
				{
					sendPacket(i, &p_put);
				}
			}
		}
		// �α��� ��, ���� ���� �ٸ� Ŭ�� ��ġ ������ �ڽſ��� Send
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (clients[i].connected)
			{
				if (i != key) 
				{
					p_put.id = i;
					p_put.x = clients[i].playerinfo->m_Pos.x;
					p_put.y = clients[i].playerinfo->m_Pos.y;
					p_put.z = clients[i].playerinfo->m_Pos.z;
					p_put.vx = clients[i].playerinfo->m_Vel.x;
					p_put.vy = clients[i].playerinfo->m_Vel.y;
					p_put.vz = clients[i].playerinfo->m_Vel.z;
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
		//cout << "[" << int(key) << "] Clients Move\n";
		cs_packet_move *p_move;
		p_move = reinterpret_cast<cs_packet_move*>(buffer);

		float distance = 1.f;

		if (p_move->flag & 0x01) {  // forward
			clients[key].playerinfo->m_Vel.z = -1.f;
			//clients[key].playerinfo->m_Pos.z += distance;
		}
		if (p_move->flag & 0x02) {	// backward
			clients[key].playerinfo->m_Vel.z = 1.f;
			//clients[key].playerinfo->m_Pos.z -= distance;
		}
		if (p_move->flag & 0x04) {	// left
			clients[key].playerinfo->m_Vel.x = 1.f;
			//clients[key].playerinfo->m_Pos.x -= distance;
		}
		if (p_move->flag & 0x08) {	// right
			clients[key].playerinfo->m_Vel.x = -1.f;
			//clients[key].playerinfo->m_Pos.x += distance;
		}

		clients[key].playerinfo->m_Vel = Normalize(clients[key].playerinfo->m_Vel);

		sc_packet_pos p_pos;
		p_pos.id = key;
		p_pos.x = clients[key].playerinfo->m_Pos.x;
		p_pos.y = clients[key].playerinfo->m_Pos.y;
		p_pos.z = clients[key].playerinfo->m_Pos.z;
		p_pos.vx = clients[key].playerinfo->m_Vel.x;
		p_pos.vy = clients[key].playerinfo->m_Vel.y;
		p_pos.vz = clients[key].playerinfo->m_Vel.z;
		p_pos.type = SC_POS;
		p_pos.size = sizeof(sc_packet_pos);
		
		// Move�� ������ ��ε�ĳ����
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
		ULONG io_byte;
		ULONG io_key;
		OVER_EX *over_ex;

		BOOL is_error = GetQueuedCompletionStatus(g_iocp, &io_byte, (PULONG_PTR)&io_key, reinterpret_cast<LPWSAOVERLAPPED *>(&over_ex), INFINITE);

		char key = static_cast<char>(io_key);

		if (is_error == FALSE) {
			//cout << "Error in GQCS\n";
			//cout << "[" << int(key) << "] Clients Disconnect\n";
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
			continue;
		}
		if (io_byte == 0)
		{
			//cout << "[" << int(key) << "] Clients Disconnect\n";
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
			continue;
		}

		if (true == over_ex->is_recv) {
			// ��Ŷ ����
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
			//cout << "[" << int(key) << "] Clients Send Packet\n";
			delete over_ex;
		}
	}
}

void do_accept()
{
	// 1. ���ϻ���  
	SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	// ������ ���ڷ� �񵿱���̶�� ���� �˷��ִ� WSA_FLAG_OVERLAPPED
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error - Invalid socket\n";
		return;
	}

	// �������� ��ü����
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 2. ���ϼ���
	if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		printf("Error - Fail bind\n");
		// 6. ��������
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

	// 3. ���Ŵ�⿭����
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Error - Fail listen\n");
		// 6. ��������
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
		// listenSocket�� �񵿱⿩�� clientSocket�� �񵿱�������� �ȴ�
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "Error - Accept Failure\n";
			return;
		}

		start = true;

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

	// 6-2. ���� ��������
	closesocket(listenSocket);

	// Winsock End
	WSACleanup();

	return;
}

void clientInputProcess()
{
	for (char i = 0; i < MAX_USER; ++i)
	{
		if (clients[i].connected == true)
		{
			PxVec3 direction = clients[i].playerinfo->m_Vel;
			float elapsedTime = gGameTimer.GetTimeElapsed();

			PxVec3 distance = -direction * elapsedTime * 20.f;

			PxControllerFilters filters;
			clients[i].playerinfo->m_PlayerController->move(distance, 0, 1/60, filters);
		}
	}
}

void clientUpdateProcess()
{
	for (char i = 0; i < MAX_USER; ++i)
	{
		if (clients[i].connected == true)
		{
			PxExtendedVec3 position = clients[i].playerinfo->m_PlayerController->getFootPosition();
			//cout << position.x << "," << position.y << "," << position.z << endl;

			clients[i].playerinfo->m_Pos = PXExtoPX(position);
		}
	}
}

void broadcastPosPacket()
{
	for (char i = 0; i < MAX_USER; ++i)
	{
		if (clients[i].connected == true)
		{
			//clients[i].playerinfo->m_Vel = Normalize(clients[i].playerinfo->m_Vel);

			sc_packet_pos p_pos;
			p_pos.id = i;
			p_pos.x = clients[i].playerinfo->m_Pos.x;
			p_pos.y = clients[i].playerinfo->m_Pos.y;
			p_pos.z = clients[i].playerinfo->m_Pos.z;
			p_pos.vx = clients[i].playerinfo->m_Vel.x;
			p_pos.vy = clients[i].playerinfo->m_Vel.y;
			p_pos.vz = clients[i].playerinfo->m_Vel.z;
			p_pos.type = SC_POS;
			p_pos.size = sizeof(sc_packet_pos);
			
			for (char j = 0; j < MAX_USER; ++j)
			{
				if (clients[j].connected == true)
				{
					cout << p_pos.x << "," << p_pos.y << "," << p_pos.z << endl;
					sendPacket(j, &p_pos);
				}
			}
		}
	}
}

void logic()
{
	float PosBroadCastTime = 0.0f;

	gGameTimer.Reset();
	while (true)
	{
		if (start)
		{
			gGameTimer.Tick(60.0f);
			PosBroadCastTime += 1.f / 60.f;

			clientInputProcess();

			gPhysx->m_Scene->simulate(1.f / 60.f);
			gPhysx->m_Scene->fetchResults(true);

			clientUpdateProcess();

			if (PosBroadCastTime > 0.1f)
			{
				broadcastPosPacket();
				PosBroadCastTime = 0.0f;
			}
		}
	}
}

void mapLoad()
{
	ifstream in("map.txt");
	int i;
	float f;
	PxVec3 vertex;

	while (true) {
		in >> i;
		int ver_size = i;
		for (int j = 0; j < ver_size; ++j) {
			in >> f;
			vertex.x = f;
			in >> f;
			vertex.y = f;
			in >> f;
			vertex.z = f;

			gMapVertex.push_back(vertex);
		}

		in >> i;
		int index_size = i;
		for (int j = 0; j < index_size; ++j) {
			in >> i;
			gMapIndex.push_back(i);
		}
		break;
	}
	in.close();

	PxTriangleMesh* triMesh = gPhysx->GetTriangleMesh(gMapVertex, gMapIndex);

	PxVec3 scaleTmp = PxVec3(1.0f, 1.0f, 1.0f);

	PxMeshScale PxScale;
	PxScale.scale = scaleTmp;

	PxTriangleMeshGeometry meshGeo(triMesh, PxScale);
	PxTransform location(0, 0, 0);

	PxMaterial* mat = gPhysx->m_Physics->createMaterial(0.2f, 0.2f, 0.2f);

	PxRigidActor* m_Actor = PxCreateStatic(*gPhysx->m_Physics, location, meshGeo, *mat);
	gPhysx->m_Scene->addActor(*m_Actor);
}

void aniLoad()
{
	ifstream in("ani.txt");
	int i;
	float f;
	
	while (in) {
		in >> i;
		in >> f;

		aniInfo.push_back(make_pair(i, f));
	}
	in.close();

	//for (auto d : aniInfo)
	//{
	//	cout << d.first << " : " << d.second << endl;
	//}
}

int main()
{
	gPhysx = new CPhysx;
	gPhysx->initPhysics();

	mapLoad();
	aniLoad();
	
	vector<thread> worker_threads;
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	// Winsock Start - windock.dll �ε�
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "Error - Can not load 'winsock.dll' file\n";
		return 0;
	}

	for (int i = 0; i < 4; ++i)
		worker_threads.push_back(thread{ worker_thread });

	thread accept_thread{ do_accept };

	thread logic_thread{ logic };

	for (auto &th : worker_threads) th.join();
	accept_thread.join();
	logic_thread.join();

	//while (true);
}