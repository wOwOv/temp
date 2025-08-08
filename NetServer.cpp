#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winmm.lib")

#include "NetServer.h"
#include "SerialBuffer.h"
#include <ws2tcpip.h>
#include <process.h>
#include <iostream>
#include <conio.h>
#include <unordered_map>
#include "Profiler.h"
#include "CPacket.h"

LONG64 NSENDPACKET;
unsigned long long Nspcall;

LONG64 NDOSEND;
LONG64 Nmsgcount;
unsigned long long Ndscall;


NetServer::NetServer() :IndexArray(0, false, true)
{
	timeBeginPeriod(1);

}

NetServer::~NetServer()
{
	//delete SessionArray;
	//delete Thread;
	timeEndPeriod(1);
}


void NetServer::Parsing(const char* txtname)
{
	//NetServer_Config.txt �Ľ��ؿ���

//txtfile open
	FILE* file = fopen(txtname, "rb");
	if (file == NULL)
	{
		printf("fopen error\n");
	}
	//���ۿ� ����
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	char* buffer = new char[size];
	rewind(file);
	size_t frerror = fread(buffer, size, 1, file);
	if (frerror == 0)
	{
		printf("fread error\n");
	}
	fclose(file);
	char* ptr = buffer;
	char ip[16];
	char port[6];
	char create[5];
	char running[5];
	char nagle[2];
	char max[7];
	int cnt = 0;

	//ip parsing
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		ip[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	ip[cnt] = '\0';

	//port parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		port[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	port[cnt] = '\0';

	//WorkerThreadCreate parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		create[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	create[cnt] = '\0';

	//WorkerThreadRunning parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		running[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	running[cnt] = '\0';

	//Nagle parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		nagle[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	nagle[cnt] = '\0';

	//MaxUser parsing
	cnt = 0;
	while (*ptr != ':')
	{
		ptr++;
	}
	ptr++;
	while (*ptr != 0x0d)
	{
		max[cnt] = *ptr;
		cnt++;
		ptr++;
	}
	max[cnt] = '\0';

	//parsing�� �͵� ���ڷ� �ٲ㼭 �ֱ�
	ZeroMemory(&_serveraddr, sizeof(_serveraddr));
	_serveraddr.sin_family = AF_INET;
	int ineterror = inet_pton(AF_INET, ip, &_serveraddr.sin_addr);
	if (ineterror != 1)
	{
		printf("inet_pton error\n");
	}
	_serveraddr.sin_port = htons(atoi(port));
	_workerthread = atoi(create);
	_concurrent = atoi(running);
	_nagleval = atoi(nagle);
	_maxuser = atoi(max);

	delete buffer;
}

//�޸�Ǯ ���ù�� Setting
int NetServer::Setting()
{
	int cpretval;//CreateIoCompletionProt retval

	//�ִ������ڼ��� �迭 ����
	SessionArray = new SESSION[_maxuser];

	//�ε��� �޸�Ǯ maxcount�����ϱ�
	IndexArray.SetMaxCount(_maxuser);

	//�޸�Ǯ�� ��ü ��� �о�ֱ�
	for (int i = 0; i < _maxuser; i++)
	{
		InitializeCriticalSection(&SessionArray[i].sendlock);
	}

	int** temp = new int* [_maxuser];
	for (int i = _maxuser - 1; i >= 0; i--)
	{
		temp[i] = IndexArray.Alloc();
		*temp[i] = i;
	}
	for (int i = _maxuser - 1; i >= 0; i--)
	{
		IndexArray.Free(temp[i]);
	}
	SessionCount = 0;

	delete[] temp;

	//iocp ����
	//CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	//����� �Ϸ� ��Ʈ ����
	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, si.dwNumberOfProcessors - _concurrent);
	if (hcp == NULL)
	{
		cpretval = GetLastError();
		printf("CreateIoCompletionPort create error: %d\n", cpretval);
		return -1;
	}



	//_workerthread���� �۾��� ������ ����
	Thread = new HANDLE[_workerthread + 2];
	for (int i = 0; i < _workerthread; i++)
	{
		Thread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, this, 0, NULL);
		if (Thread[i] == NULL)
		{
			return 1;
		}
	}

	//MonitorThread����
	Thread[_workerthread + 1] = (HANDLE)_beginthreadex(NULL, 0, &MonitorThread, this, 0, NULL);
	if (Thread[_workerthread] == NULL)
	{
		return 1;
	}

	return 0;

}

int NetServer::Network()
{
	//retval
	int scretval;//startup
	int lscretval;//listensock retval
	int sbretval;//setsockopt sndbuf0 retval
	int bdretval;//bind retval
	int lnretval;//listen retval
	int lgretval;//linger retval
	int ndretval;//nodelay retval

	//���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		scretval = WSAGetLastError();
		printf("WSAStartup error: %d\n", scretval);
		return -1;
	}

	//socket()
	ListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSock == INVALID_SOCKET)
	{
		lscretval = WSAGetLastError();
		printf("socket() error: %d\n", lscretval);
		return -1;
	}


	//bind()
	bdretval = bind(ListenSock, (SOCKADDR*)&_serveraddr, sizeof(_serveraddr));
	if (bdretval == SOCKET_ERROR)
	{
		bdretval = WSAGetLastError();
		printf("bind error: %d\n", bdretval);
		return -1;
	}

	//SO_SNDBUF
	int bfoptval = 0;
	sbretval = setsockopt(ListenSock, SOL_SOCKET, SO_SNDBUF, (char*)&bfoptval, sizeof(bfoptval));
	if (sbretval == SOCKET_ERROR)
	{
		sbretval = WSAGetLastError();
		printf("nonblocking error: %d\n", sbretval);
		return -1;
	}

	//SO_LINGER
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	lgretval = setsockopt(ListenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (lgretval == SOCKET_ERROR)
	{
		lgretval = WSAGetLastError();
		printf("linger error: %d\n", lgretval);
		return -1;
	}

	//nodelay
	if (_nagleval != 0)
	{
		BOOL ndoptval = TRUE;
		ndretval = setsockopt(ListenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&ndoptval, sizeof(ndoptval));
		if (ndretval == SOCKET_ERROR)
		{
			ndretval = WSAGetLastError();
			printf("nodelay error: %d\n", ndretval);
			return -1;
		}
	}

	//listen()
	lnretval = listen(ListenSock, SOMAXCONN_HINT(20000));
	if (lnretval == SOCKET_ERROR)
	{
		lnretval = WSAGetLastError();
		printf("listen error: %d\n", lnretval);
		return -1;
	}

	Thread[_workerthread] = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread, this, 0, NULL);



	return 0;
}

bool NetServer::Start(const char* txtname)
{
	//config parsing�Ͽ� �ʿ��� ������ ������
	Parsing(txtname);
	Setting();
	Network();

	return 1;
}

void NetServer::Stop()
{
	int scretval;//cleanup retval
	int csretval;//closesocket retval

	//accept������ ������ ���� closesocket
	csretval = closesocket(ListenSock);
	if (csretval == SOCKET_ERROR)
	{
		csretval = WSAGetLastError();
		printf("closesocket error: %d\n", csretval);
	}

	//��Ŀ������ ������ ���� postqueue
	ULONG_PTR p = 0;
	LPOVERLAPPED po = nullptr;
	for (int i = 0; i < _workerthread; i++)
	{
		PostQueuedCompletionStatus(hcp, 0, p, po);
	}

	//monitorThread����Ǳ� ���->monitorthread�� accept������,��Ŀ������� �� ����� �� �����ϱ⿡
	WaitForSingleObject(Thread[_workerthread + 1], INFINITE);
	printf("All thread ended!\n");

	//��������
	scretval = WSACleanup();
	if (scretval == SOCKET_ERROR)
	{
		scretval = WSAGetLastError();
		printf("cleanup error: %d\n", scretval);
	}
}

int NetServer::GetSessionCount()
{
	return SessionCount;
}

bool NetServer::Disconnect(__int64 sessionID)
{
	//����idã��
	int index = FindSession(sessionID);

	IOCount* check;
	long temp = InterlockedIncrement((long*)&SessionArray[index].iocount);
	check = (IOCount*)&temp;

	//rFlag�� true���ٸ�
	if (check->rFlag == 0xffff)
	{
		if (InterlockedDecrement((long*)&SessionArray[index].iocount) == 0)
		{
			Release(&SessionArray[index]);
		}
		return false;
	}

	//rFlag�� false��� remove flag�ٲٰ� release�ϵ��� ����
	InterlockedExchange(&SessionArray[index].remove, 1);
	CancelIoEx((HANDLE)SessionArray[index].sock, NULL);

	if (InterlockedDecrement((long*)&SessionArray[index].iocount) == 0)
	{
		Release(&SessionArray[index]);
	}

	return true;
}

bool NetServer::SendPacket(__int64 sessionID, CPacket packet)
{
	int index = FindSession(sessionID);

	IOCount* check;
	long temp = InterlockedIncrement((long*)&SessionArray[index].iocount);
	check = (IOCount*)&temp;

	//rFlag�� true���ٸ�
	if (check->rFlag == 0xffff)
	{
		if (InterlockedDecrement((long*)&SessionArray[index].iocount) == 0)
		{
			Release(&SessionArray[index]);
		}
		return false;
	}

	if (SessionArray[index].button == 1)
	{
		if (SessionArray[index].remove == 0)
		{
			SBuffer* msgbuf = packet.SBuf;
			msgbuf->AddRefcnt(1);	//���� msgbuf���� ����

			///��������ϴ� �͵� �ѹ��� �Ǿ���ϴ°žƴѰ�
			SetBufferHeader(msgbuf);
			if (msgbuf->eFlag == 0)
			{
				AcquireSRWLockExclusive(&msgbuf->eKey);
				if (msgbuf->eFlag == 0)
				{
					Encode(msgbuf);
					InterlockedExchange8(&msgbuf->eFlag, 1);
				}
				ReleaseSRWLockExclusive(&msgbuf->eKey);
			}
			msgbuf->AddRefcnt(1);	//SendQ�� ��ť�ҰŴϱ�
			SessionArray[index].SendQ.Enqueue(msgbuf);

			msgbuf->DecRefcnt();	//���� msgbuf �� ��
			InterlockedIncrement(&SessionArray[index].message);

			DoSend(&SessionArray[index]);
		}
	}

	if (InterlockedDecrement((long*)&SessionArray[index].iocount) == 0)
	{
		Release(&SessionArray[index]);
	}

	return true;
}


int NetServer::getAcceptTPS()
{
	return AcceptTPS;
}

int NetServer::getRecvMessageTPS()
{
	return RecvMessageTPS;
}

int NetServer::getSendMessageTPS()
{
	return SendMessageTPS;
}

//�۾��� ������ �Լ�
unsigned __stdcall NetServer::WorkerThread(LPVOID arg)
{
	NetServer* netserver = (NetServer*)arg;
	int gqcsretval;//GetQueuedCompletionStatus retval;
	HANDLE iocp = netserver->hcp;

	while (1)
	{
		//�񵿱� ����� �Ϸ� ��ٸ���
		DWORD cbTransferred = 0;
		myOverlapped* myoverlapped = 0;
		SESSION* tgt = 0;
		gqcsretval = GetQueuedCompletionStatus(iocp, &cbTransferred, (PULONG_PTR)&tgt, (LPOVERLAPPED*)&myoverlapped, INFINITE);

		//�񵿱� ����� ��� Ȯ��
		//������ ����
		if (cbTransferred == 0 && tgt == 0 && myoverlapped == 0)
		{
			printf("workerThread ended\n");
			return 0;

		}
		//
		if (gqcsretval == 0 || cbTransferred == 0)
		{
			if (gqcsretval == 0)
			{
				gqcsretval = WSAGetLastError();
				DWORD temp1, temp2;
				WSAGetOverlappedResult(tgt->sock, &myoverlapped->overlapped, &temp1, FALSE, &temp2);
				//printf("GetQueuedCompletionStatus error : %d\n", gqcsretval);
			}

		}
		else
		{

			//recvio overlapped��
			if (myoverlapped->type == 0)
			{
				EnterCriticalSection(&tgt->sendlock);
				netserver->RecvCompletion(tgt, cbTransferred);
				LeaveCriticalSection(&tgt->sendlock);
				netserver->DoRecv(tgt);
			}
			if (myoverlapped->type == 1)
			{
				EnterCriticalSection(&tgt->sendlock);
				netserver->SendCompletion(tgt, cbTransferred);
				InterlockedExchange(&tgt->sendflag, 0);
				netserver->DoSend(tgt);
				LeaveCriticalSection(&tgt->sendlock);

			}


		}

		//�Ϸ����� �� �Ϳ� ���� iocount����
		if (InterlockedDecrement((long*)&tgt->iocount) == 0)
		{
			netserver->Release(tgt);
		}


	}
}

//accept ������ �Լ� Thread[_workerthread]
unsigned __stdcall NetServer::AcceptThread(LPVOID arg)
{
	NetServer* netserver = (NetServer*)arg;
	int atretval;//accept retval
	int rvretval;//recv retval

	//������ ��ſ� ����� ����
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes;
	DWORD flags;

	while (1)
	{

		//accept()
		SOCKET sock;
		addrlen = sizeof(clientaddr);
		sock = accept(netserver->ListenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (sock == INVALID_SOCKET)
		{
			atretval = WSAGetLastError();
			printf("accept error: %d\n", atretval);
			printf("AcceptThread ended\n");
			return -1;
		}

		else
		{
			InterlockedIncrement(&netserver->acceptcount);
			int* indexret = netserver->GetVoidSession();
			//���� ���� ����ü �Ҵ�
			if (indexret == nullptr)
			{
				closesocket(sock);
				printf("SessionArray full\n");
				continue;
			}

			SESSION* tgt = &netserver->SessionArray[*indexret];
			InterlockedIncrement(&netserver->SessionCount);
			ZeroMemory(&tgt->sendio.overlapped, sizeof(tgt->sendio.overlapped));
			ZeroMemory(&tgt->recvio.overlapped, sizeof(tgt->recvio.overlapped));

			InterlockedExchange(&tgt->button, 1);

			tgt->sendio.type = 1;
			tgt->recvio.type = 0;

			tgt->RecvQ.ClearBuffer();
			netserver->ClearSendQ(&tgt->SendQ);

			tgt->sock = sock;
			tgt->ip = clientaddr.sin_addr;
			tgt->port = clientaddr.sin_port;
			__int64 temp = *indexret;
			//sessionID�� index�����
			tgt->sessionID = netserver->SessionKey | temp << 48;


			tgt->sendflag = 0;
			tgt->iocount.iocount++;
			tgt->iocount.rFlag = 0;

			tgt->index = indexret;

			InterlockedExchange(&tgt->remove, 0);



			netserver->SessionKey++;

			//���ϰ� ����� �Ϸ� ��Ʈ ����
			CreateIoCompletionPort((HANDLE)tgt->sock, netserver->hcp, (ULONG_PTR)tgt, 0);

			netserver->OnAccept(&clientaddr, tgt->sessionID);

			//�񵿱� ����� ����
			WSABUF wsabuf;
			wsabuf.buf = tgt->RecvQ.GetRearBufferPtr();
			wsabuf.len = tgt->RecvQ.DirectEnqueueSize();
			recvbytes = 0;
			flags = 0;

			rvretval = WSARecv(tgt->sock, &wsabuf, 1, &recvbytes, &flags, &tgt->recvio.overlapped, NULL);
			if (rvretval == SOCKET_ERROR)
			{
				rvretval = WSAGetLastError();
				if (rvretval != ERROR_IO_PENDING)
				{

					if (InterlockedDecrement((long*)&tgt->iocount) == 0)
					{
						netserver->Release(tgt);
					}
				}

			}


		}
	}
}

//Monitor ������ �Լ� Thread[_workerthread+1]
unsigned __stdcall NetServer::MonitorThread(LPVOID arg)
{
	NetServer* netserver = (NetServer*)arg;

	netserver->timecheck = timeGetTime();
	while (1)
	{
		//accpet������� ��Ŀ������� ����Ǹ� ����ͽ����嵵 �����ϱ�
		int waitret = WaitForMultipleObjects(netserver->_workerthread + 1, netserver->Thread, TRUE, 0);
		if (waitret == WAIT_OBJECT_0)
		{
			return 0;
		}

		//1�� �Ǹ� �ʱ�ȭ
		if (timeGetTime() - netserver->timecheck >= 1000)
		{
			netserver->timecheck += 1000;
			netserver->AcceptTPS = netserver->acceptcount;
			InterlockedExchange(&netserver->acceptcount, 0);
			netserver->RecvMessageTPS = netserver->recvcount;
			InterlockedExchange(&netserver->recvcount, 0);
			netserver->SendMessageTPS = netserver->sendcount;
			InterlockedExchange(&netserver->sendcount, 0);
		}


	}
}

//���� ��Ȱ��ȭ�ϱ�
//�޸�Ǯ ���ù��
bool NetServer::Release(SESSION* tgt)
{
	long flag = 0xffff0000;
	if (InterlockedCompareExchange((long*)&tgt->iocount, flag, 0) != 0)
	{
		return false;
	}

	closesocket(tgt->sock);
	InterlockedExchange(&tgt->button, 0);
	for (int i = 0; i < tgt->packetbox.count; i++)
	{
		tgt->packetbox.SBufferArray[i]->DecRefcnt();

	}
	//�޸�Ǯ ���� ���
	IndexArray.Free(tgt->index);
	InterlockedDecrement(&SessionCount);
	OnRelease(tgt->sessionID);
	return true;

}

void NetServer::RecvCompletion(SESSION* tgt, DWORD cbTransferred)
{
	tgt->RecvQ.MoveRear(cbTransferred);

	//printf("WSARecv completion\n");

	while (1)
	{
		//recv �� ó��
		//header��ŭ ���Դ��� Ȯ��

		if (tgt->RecvQ.GetUsedSize() < sizeof(NetHEADER))
		{
			break;
		}

		NetHEADER header;
		int pkret = tgt->RecvQ.Peek((char*)&header, sizeof(NetHEADER));
		if (pkret != sizeof(NetHEADER))
		{
			DebugBreak();
		}

		//���۵� �޽�����
		if (header.code != _code)
		{
			Disconnect(tgt->sessionID);
			return;
		}

		//���+������ ��ŭ ����ִ��� Ȯ��
		if (tgt->RecvQ.GetUsedSize() < header.len + sizeof(NetHEADER))
		{
			break;
		}
		SBuffer* msgbuf = SBuffer::BufPool.Alloc();
		msgbuf->AddRefcnt(1);
		msgbuf->ClearAtNServer();
		int deqret = tgt->RecvQ.Dequeue(msgbuf->GetWritePtr(), header.len + sizeof(NetHEADER));
		if (deqret != header.len + sizeof(NetHEADER))
		{
			DebugBreak();
		}
		int movret = msgbuf->MoveWritePos(deqret);
		if (movret != deqret)
		{
			DebugBreak();
		}
		InterlockedIncrement(&recvcount);

		//���ڵ�
		bool check = Decode(msgbuf);

		//�߸��� �޽�����
		if (check == false)
		{
			Disconnect(tgt->sessionID);
		}
		else
		{
			int movrret = msgbuf->MoveReadPos(sizeof(NetHEADER));
			if (movrret != sizeof(NetHEADER))
			{
				DebugBreak();
			}

			CPacket packet(msgbuf);
			OnMessage(tgt->sessionID, packet);
		}
		msgbuf->DecRefcnt();
	}

}

bool NetServer::DoRecv(SESSION* tgt)
{
	int rvretval;
	//WSARecv �ɱ�
	InterlockedIncrement((long*)&tgt->iocount);
	if (tgt->RecvQ.GetUsedSize() == tgt->RecvQ.DirectEnqueueSize())
	{
		WSABUF wsabuf;
		wsabuf.buf = tgt->RecvQ.GetRearBufferPtr();
		wsabuf.len = tgt->RecvQ.DirectEnqueueSize();
		DWORD recvbytes, flags = 0;
		ZeroMemory(&tgt->recvio.overlapped, sizeof(tgt->recvio.overlapped));


		rvretval = WSARecv(tgt->sock, &wsabuf, 1, &recvbytes, &flags, &tgt->recvio.overlapped, NULL);
		if (rvretval == SOCKET_ERROR)
		{
			rvretval = WSAGetLastError();
			if (rvretval != ERROR_IO_PENDING)
			{

				if (InterlockedDecrement((long*)&tgt->iocount) == 0)
				{
					Release(tgt);
				}
				return false;
			}
		}
		return true;
	}
	else
	{
		WSABUF wsabuf[2];
		DWORD recvbytes = 0;
		DWORD recvflags = 0;
		wsabuf[0].buf = tgt->RecvQ.GetRearBufferPtr();
		wsabuf[0].len = tgt->RecvQ.DirectEnqueueSize();
		wsabuf[1].buf = tgt->RecvQ.GetStartBufferPtr();
		wsabuf[1].len = tgt->RecvQ.GetFreeSize() - wsabuf[0].len;
		ZeroMemory(&tgt->recvio.overlapped, sizeof(tgt->recvio.overlapped));

		rvretval = WSARecv(tgt->sock, wsabuf, 2, &recvbytes, &recvflags, &tgt->recvio.overlapped, NULL);
		if (rvretval == SOCKET_ERROR)
		{
			rvretval = WSAGetLastError();
			if (rvretval != ERROR_IO_PENDING)
			{

				if (InterlockedDecrement((long*)&tgt->iocount) == 0)
				{
					Release(tgt);
				}
				return false;
			}
		}
		return true;
	}
}

void NetServer::SendCompletion(SESSION* tgt, DWORD cbTransferred)
{
	for (int i = 0; i < tgt->packetbox.count; i++)
	{
		tgt->packetbox.SBufferArray[i]->DecRefcnt();
	}
	tgt->packetbox.count = 0;
}


bool NetServer::DoSend(SESSION* tgt)
{
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	LARGE_INTEGER time;

	int sdretval;
	if (tgt->SendQ.GetUsedSize() > 0)
	{
		if (InterlockedExchange(&tgt->sendflag, 1) == 0)
		{
			QueryPerformanceCounter(&start);
			InterlockedIncrement((long*)&tgt->iocount);
			WSABUF wsabuf[PACKETBOXMAX];
			DWORD sendbytes = 0;
			DWORD sendflags = 0;

			int size = tgt->SendQ.GetUsedSize();
			if (size > PACKETBOXMAX)
			{
				size = PACKETBOXMAX;
			}
			tgt->packetbox.count = size;

			for (int i = 0; i < size; i++)
			{
				//tgt->packetbox.SBufferArray[i] = tgt->SendQ.Dequeue();
				tgt->SendQ.Dequeue(&tgt->packetbox.SBufferArray[i]);
				wsabuf[i].buf = tgt->packetbox.SBufferArray[i]->GetReadPtr();
				wsabuf[i].len = tgt->packetbox.SBufferArray[i]->GetDataSize();
			}
			ZeroMemory(&tgt->sendio.overlapped, sizeof(tgt->sendio.overlapped));
			sdretval = WSASend(tgt->sock, wsabuf, size, &sendbytes, sendflags, &tgt->sendio.overlapped, NULL);
			if (sdretval == SOCKET_ERROR)
			{
				sdretval = WSAGetLastError();
				if (sdretval != ERROR_IO_PENDING)
				{

					if (InterlockedDecrement((long*)&tgt->iocount) == 0)
					{
						Release(tgt);
					}
					return false;
				}
			}
			QueryPerformanceCounter(&end);
			time.QuadPart = end.QuadPart - start.QuadPart;
			InterlockedAdd64(&NDOSEND, time.QuadPart);
			InterlockedIncrement(&Ndscall);
			InterlockedAdd64(&Nmsgcount, size);

			InterlockedAdd(&sendcount, tgt->message);
			InterlockedExchange(&tgt->message, 0);
			return true;

		}
	}
	return false;
}



//������� �ʰ� �ִ� SessionArray ����� index�� return
//�޸�Ǯ ���ù��
int* NetServer::GetVoidSession()
{
	int* temp = IndexArray.Alloc();
	return temp;
}




//SessionID�� �ش��ϴ� ��Ҹ� SessionArray���� ã���� �ε��� return
//SessionID�� ������ �ε��� �ֱ�
int NetServer::FindSession(__int64 tgtID)
{
	return tgtID >> 48;
}

void NetServer::SetBufferHeader(SBuffer* msgbuf)
{

	NetHEADER header;
	header.code = _code;
	header.len = msgbuf->GetDataSize();
	header.randkey = 0;//rand�Լ� ������
	header.checksum = 0;
	for (int i = 0; i < header.len; i++)
	{
		header.checksum += msgbuf->buffer[5 + i];
	}
	header.checksum %= 256;

	msgbuf->read = 0;
	msgbuf->DataSize += 5;
	NetHEADER* temp = (NetHEADER*)msgbuf->GetReadPtr();
	*temp = header;

}

void NetServer::Encode(SBuffer* msg)
{
	unsigned short len = *(short*)&msg->buffer[1];
	unsigned char randkey = *(char*)&msg->buffer[3];

	char D;
	char P1 = 0;
	char E1 = 0;
	char P2 = 0;
	char E2 = 0;
	for (int i = 0; i < len+1; i++)
	{
		D = msg->buffer[4 + i];
		P2 = D ^ (P1 + randkey + i + 1);
		E2 = P2 ^ (E1 + _fixedKey + i + 1);
		msg->buffer[4 + i] = E2;
		P1 = P2;
		E1 = E2;
	}
}

bool NetServer::Decode(SBuffer* msg)
{
	unsigned short len = *(short*)&msg->buffer[1];
	unsigned char randkey = *(char*)&msg->buffer[3];

	char P1 = 0;
	char E1 = 0;
	char P2 = 0;
	char E2 = 0;
	for (int i = 0; i < len+1; i++)
	{
		E2 = msg->buffer[4 + i];
		P2 = E2 ^ (E1 + _fixedKey + i + 1);
		msg->buffer[4 + i] = P2 ^ (P1 + randkey + i + 1);
		P1 = P2;
		E1 = E2;
	}

	//checksum Ȯ��
	unsigned char checksum= *(char*)&msg->buffer[4];
	unsigned char msgchecksum = 0;
	for (int i = 0; i < len; i++)
	{
		msgchecksum+= msg->buffer[5 + i];
	}
	msgchecksum %= 256;

	if (checksum != msgchecksum)
	{
		return false;
	}

	return true;
}


void NetServer::ClearSendQ(LFQueue<SBuffer*>* lfQ)
{
	SBuffer* tempbuf;
	bool check;
	while (1)
	{
		check = lfQ->Dequeue(&tempbuf);
		if (check == false)
		{
			break;
		}
		tempbuf->DecRefcnt();
	}
}
