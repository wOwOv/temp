
#ifndef __CHATSERVER__
#define __CHATSERVER__

#include "CoreServer.h"
#include "TlsMemoryPool.h"
#include "LockFreeQueue.h"
#include <thread>
#include <mutex>

#define dfPACKET_CODE		0x77
#define dfPACKET_KEY		0x32

#define SECTOR_MAX_X 50
#define SECTOR_MAX_Y 50

enum en_PACKET_TYPE
{
	en_PACKET_CS_CHAT_SERVER = 0,
	en_PACKET_CS_CHAT_REQ_LOGIN,
	en_PACKET_CS_CHAT_RES_LOGIN,
	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,
	en_PACKET_CS_CHAT_RES_SECTOR_MOVE,
	en_PACKET_CS_CHAT_REQ_MESSAGE,
	en_PACKET_CS_CHAT_RES_MESSAGE,
	en_PACKET_CS_CHAT_REQ_HEARTBEAT,
};



class ChatServer:public CoreServer
{
public:
	void PrintSectorCount();
	void PrintLoginError();

private:
	enum {ACCEPT=0,MESSAGE,RELEASE,HEART_O,HEART_X};

	struct ToDo
	{
		char type;
		__int64 sessionId;
		SOCKADDR_IN clientaddr;
		CPacket* packet;
	};

	struct ChatUser
	{
		__int64 sessionId;
		SOCKADDR_IN clientaddr;
		INT64	accountNo;
		WCHAR	id[20];				// null 포함
		WCHAR	nickname[20];		// null 포함
		char	sessionKey[64];		// 인증토큰
		signed char	sectorX;
		signed char	sectorY;
		std::chrono::steady_clock::time_point heartbeat;

	};
	struct SectorAround
	{
		int count;
		int around[9][2]; //0: X, 1: Y
	};
public:
	ChatServer();
	~ChatServer();
	
	void ServerControl();

	//Monitoring
public:
	unsigned long GetToDoPoolCapacity();
	unsigned long GetToDoQueueRemained();
	
	unsigned long GetChatUserPoolCapacity();
	unsigned long GetUnLoginCount();
	unsigned long GetChatUserCount();


	unsigned long long GetAcceptTotal();
	unsigned long GetAcceptTPS();
	unsigned long GetLogicTPS();
	unsigned long GetLoginTPS();
	unsigned long GetSectorTPS();
	unsigned long GetMessageTPS();
	unsigned long GetHeartTPS();

private:
	virtual bool OnConnectionRequest(SOCKADDR_IN* clientaddr) override;
	virtual void OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID) override;
	virtual void OnMessage(__int64 sessionID, CPacket packet) override;
	virtual void OnRelease(__int64 sessionID) override;

	//스레드
	static unsigned __stdcall LogicThread(LPVOID arg);
	static unsigned __stdcall TimerThread(LPVOID arg);
	static unsigned __stdcall MonitorThread(LPVOID arg);

private:
	void ProcAccept(ToDo* job);
	void ProcMessage(ToDo* job);
	void ProcRelease(ToDo* job);
	void ProcHeartO(ToDo* job);
	void ProcHeartX(ToDo* job);

	void Proc_REQ_LOGIN(__int64 sessionId,CPacket packet);
	void MP_RES_LOGIN(CPacket* packet, BYTE status, INT64 accountno);
	void Proc_REQ_SECTOR_MOVE(__int64 sessionId, CPacket packet);
	void MP_RES_SECTOR_MOVE(CPacket* packet, INT64 accontno, WORD sectorx, WORD sectory);
	void Proc_REQ_MESSAGE(__int64 sessionId, CPacket packet);
	void MP_RES_MESSAGE(CPacket* packet, INT64 accountno, WCHAR id[], WCHAR nickname[], WORD len, WCHAR msg[]);
	void Proc_REQ_HEARTBEAT(__int64 sessionId, CPacket packet);


private:
	bool LoginProcess();
	
	void GetSectorAround(int SectorX, int SectorY, SectorAround* sectoraround);

private:
	std::thread _LogicThread;
	std::thread _TimerThread;
	std::thread _MonitorThread;
	//HANDLE _LogicThread;										//로직스레드
	//HANDLE _TimerThread;										//하트비트용 타이머스레드
	LFQueue<ToDo*> _todoQ;										//iocp와 LogicThread연결
	TlsMemoryPool<ToDo> _todopool;								//ToDo풀
	std::condition_variable _todoEvent;
	//HANDLE _todoEvent;											//todoQ이벤트
	std::mutex _todoM;

	std::unordered_map<INT64, ChatUser*> _UnLoginMap;			//로그인 전 user관리
	std::unordered_map<INT64, ChatUser*> _ChatUserMap;			//로그인 후 user관리
	std::list<ChatUser*> _Sector[SECTOR_MAX_Y][SECTOR_MAX_X];	//섹터관리
	MemoryPool<ChatUser> _userpool;								//ChatUser풀

	unsigned int _timeout=40;
	
	//모니터링
	unsigned long long _acceptCall=0;
	unsigned long _acceptTPS = 0;
	unsigned long _acceptcount = 0;
	unsigned long _logicTPS = 0;
	unsigned long _logiccount = 0;
	unsigned long _MLoginTPS = 0;
	unsigned long _logincount = 0;
	unsigned long _MSectorTPS = 0;
	unsigned long _sectorcount = 0;
	unsigned long _MMessageTPS = 0;
	unsigned long _messagecount = 0;
	unsigned long _MHeartbeatTPS = 0;
	unsigned long _heartcount = 0;

	unsigned long long _loginerror = 0;
};

#endif
