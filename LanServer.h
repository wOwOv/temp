#ifndef __CORESERVER__
#define __CORESERVER__

#include <unordered_map>
#include <winsock2.h>
#include "RingBuffer.h"
#include "LockFreeQueue.h"
#include "MemoryPool.h"
#include "CPacket.h"
#include "SerialBuffer.h"

class SBuffer;

#define PACKETBOXMAX 1000

class LanServer
{
private:
	struct LanHEADER
	{
		unsigned short len;
	};
	struct myOverlapped
	{
		WSAOVERLAPPED overlapped;
		bool type;				//0이면 recv 1이면 send
	};
	struct PacketBox
	{
		SBuffer* SBufferArray[PACKETBOXMAX];
		int count;
	};
	struct IOCount
	{
		short iocount = 0;
		short rFlag = 0;
	};
	struct SESSION
	{
		myOverlapped sendio;
		myOverlapped recvio;

		RingBuffer RecvQ;
		LFQueue<SBuffer*> SendQ;

		SOCKET sock;
		IN_ADDR ip;
		u_short port;

		__int64 sessionID;

		long sendflag = 0;
		
		IOCount iocount;

		long button = 0;
		long remove = 0;

		long message = 0;//SendMessageTPS용..

		int* index;

		CRITICAL_SECTION sendlock;

		PacketBox packetbox;
	};

public:

	LanServer();
	~LanServer();

	bool Start(const char* txtname);
	void Stop();
	int GetSessionCount();
	bool Disconnect(__int64 sessionID);
	bool SendPacket(__int64 sessionID,CPacket packet);


	int getAcceptTPS();
	int getRecvMessageTPS();
	int getSendMessageTPS();


private:
	virtual bool OnConnectionRequest(SOCKADDR_IN* clientaddr) = 0;
	virtual void OnAccept(SOCKADDR_IN* clientaddr,__int64 sessionID) = 0;
	virtual void OnRelease(__int64 sessionID) = 0;
	virtual void OnMessage(__int64 sessionID, CPacket packet) = 0;
	//virtual void OnError(int errorcode, wchar* stringbox) = 0;

private:
	//Start가 쓰는 함수들
	void Parsing(const char* txtname);
	int Setting();
	int Network();
	///////////////////////////////////

	static unsigned __stdcall WorkerThread(LPVOID arg);
	static unsigned __stdcall AcceptThread(LPVOID arg);
	static unsigned __stdcall MonitorThread(LPVOID arg);

	//WorkerThread가 쓰는 함수들
	bool Release(SESSION* tgt);
	void RecvCompletion(SESSION* tgt, DWORD cbTransferred);
	bool DoRecv(SESSION* tgt);
	void SendCompletion(SESSION* tgt, DWORD cbTransferred);
	bool DoSend(SESSION* tgt);
	//////////////////////////////////////////////////////////////////////

	//AcceptThread가 쓰는 함수들
	//int GetVoidSession();
	int* GetVoidSession();
	//////////////////////////
	int FindSession(__int64 tgtID);

private:
	//직렬화버퍼 헤더 세팅
	void SetBufferHeader(SBuffer* msgbuf);
	//SendQ 락프리큐 초기화용
	void ClearSendQ(LFQueue<SBuffer*>* lfQ);

private:
	long AcceptTPS=0;
	long acceptcount = 0;
	long RecvMessageTPS=0;
	long recvcount = 0;
	long SendMessageTPS=0;
	long sendcount = 0;
	__int64 SessionKey = 1;


private:
	SOCKET ListenSock;
	HANDLE hcp;
	SESSION* SessionArray;
	DWORD SessionCount;
	DWORD timecheck;
	HANDLE* Thread;

	unsigned _code;
	unsigned char _fixedKey;

private:
	MemoryPool<int> IndexArray;

private:
	SOCKADDR_IN _serveraddr;
	int _workerthread;
	int _concurrent;
	bool _nagleval;
	int _maxuser;
};




#endif
