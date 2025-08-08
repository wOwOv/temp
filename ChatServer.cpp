#include "ChatServer.h"
#include <process.h>
#include <unordered_map>
#include <list>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <conio.h>

__int64 errorsession;

void ChatServer::PrintSectorCount()
{
	for (int i = 0; i < 50; i++)
	{
		for (int j = 0; j < 50; j++)
		{
			printf("%d ", _Sector[i][j].size());
		}
		printf("\n");
	}
}

void ChatServer::PrintLoginError()
{
	printf("%d\n", _loginerror);
}

ChatServer::ChatServer() : CoreServer(NET)
{
	//_LogicThread= (HANDLE)_beginthreadex(NULL, 0, &LogicThread, this, 0, NULL);
	_LogicThread=std::thread(LogicThread,this);
	//_TimerThread= (HANDLE)_beginthreadex(NULL, 0, &TimerThread, this, 0, NULL);
	_TimerThread = std::thread(TimerThread, this);
	//_todoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_MonitorThread = std::thread(MonitorThread, this);
}

ChatServer::~ChatServer()
{
}

void ChatServer::ServerControl()
{
	//Ű���� ��Ʈ�� ���, Ǯ�� ����
	static bool bControlMode = false;

	//------------------------------------------------
	//L: ��Ʈ�� Lock/ U: ��Ʈ�� Unlock / Q: ���� ����
	//------------------------------------------------
	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		//Ű���� ���� ���
		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			bControlMode = true;

			//���� Ű ���� ���
			wprintf(L"Control Mode : Press Q - Quit\n");
			wprintf(L"Control Mode : Press L - Key Lock\n");
		}

		//Ű���� ���� ���
		if ((L'l' == ControlKey || L'L' == ControlKey) && bControlMode)
		{
			wprintf(L"Control Lock..! Press U-Control Unlock\n");

			bControlMode = false;
		}

		//Ű���� ���� Ǯ�� ���¿��� Ư�� ���
		if ((L'q' == ControlKey || L'Q' == ControlKey) && bControlMode)
		{
			//g_bShutdown = true;
		}

		//��Ÿ ��Ʈ�� ���...

	}

}

unsigned long ChatServer::GetToDoPoolCapacity()
{
	return _todopool.GetCapacity();
}

unsigned long ChatServer::GetToDoQueueRemained()
{
	return _todoQ.GetUsedSize();
}

unsigned long ChatServer::GetChatUserPoolCapacity()
{
	return _userpool.GetCapacityCount();
}

unsigned long ChatServer::GetUnLoginCount()
{
	return _UnLoginMap.size();
}

unsigned long ChatServer::GetChatUserCount()
{
	return _ChatUserMap.size();
}




unsigned long long ChatServer::GetAcceptTotal()
{
	return _acceptCall;
}

unsigned long ChatServer::GetAcceptTPS()
{
	return _acceptTPS;
}

unsigned long ChatServer::GetLogicTPS()
{
	return _logicTPS;
}

unsigned long ChatServer::GetLoginTPS()
{
	return _MLoginTPS;
}

unsigned long ChatServer::GetSectorTPS()
{
	return _MSectorTPS;
}

unsigned long ChatServer::GetMessageTPS()
{
	return _MMessageTPS;
}

unsigned long ChatServer::GetHeartTPS()
{
	return _MHeartbeatTPS;
}

bool ChatServer::OnConnectionRequest(SOCKADDR_IN* clientaddr)
{
	return false;
}

void ChatServer::OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID)
{
	InterlockedIncrement(&_acceptCall);
	//ACCEPT job ��ť
	ToDo* job = _todopool.Alloc();
	job->type = ACCEPT;
	job->sessionId = sessionID;
	job->clientaddr = *clientaddr;
	job->packet = nullptr;

	_todoQ.Enqueue(job);
	_todoEvent.notify_one();
	//SetEvent(_todoEvent);

}

void ChatServer::OnMessage(__int64 sessionID, CPacket packet)
{
	//MESSAGE job ��ť
	ToDo* job = _todopool.Alloc();
	job->type = MESSAGE;
	job->sessionId = sessionID;
	job->packet = packet.MakePacketPtr();

	_todoQ.Enqueue(job);
	_todoEvent.notify_one();
	//SetEvent(_todoEvent);
}


void ChatServer::OnRelease(__int64 sessionID)
{
	//ACCEPT job ��ť
	ToDo* job = _todopool.Alloc();
	job->type = RELEASE;
	job->sessionId = sessionID;
	job->packet = nullptr;

	_todoQ.Enqueue(job);
	_todoEvent.notify_one();
	//SetEvent(_todoEvent);
}

unsigned __stdcall ChatServer::LogicThread(LPVOID arg)
{
	ChatServer* chatserver = (ChatServer*)arg;
	while (1)
	{
		std::unique_lock<std::mutex> lock(chatserver->_todoM);
		chatserver->_todoEvent.wait(lock);
		//WaitForSingleObject(chatserver->_todoEvent, INFINITE);

		while (1)
		{
			ToDo* job;
			bool check=chatserver->_todoQ.Dequeue(&job);
			if (check == false)
			{
				break;
			}
			switch (job->type)
			{
			case ACCEPT:
			{
				chatserver->ProcAccept(job);
				break;
			}
			case MESSAGE:
			{
				chatserver->ProcMessage(job);
				break;
			}
			case RELEASE:
			{
				chatserver->ProcRelease(job);
				break;
			}
			case HEART_O:
			{
				chatserver->ProcHeartO(job);
				break;
			}
			case HEART_X:
			{
				chatserver->ProcHeartX(job);
				break;
			}
			default:
			{
				break;
			}
			}
			chatserver->_todopool.Free(job);
		}
	}
}

unsigned __stdcall ChatServer::TimerThread(LPVOID arg)
{
	ChatServer* chatserver = (ChatServer*)arg;
	while (1)
	{

		//�α��ο�û ���� ������ ����(3��)
		for (int i = 0; i < 10; i++)
		{
			//Sleep(chatserver->_timeout);
			std::this_thread::sleep_for(std::chrono::seconds(3));
			//HEART_X job ��ť
			ToDo* job = chatserver->_todopool.Alloc();
			job->type = HEART_X;
			job->packet = nullptr;

			chatserver->_todoQ.Enqueue(job);
			chatserver->_todoEvent.notify_one();
			//SetEvent(chatserver->_todoEvent);
		}

		//�α��� �� ���������� ������ ����(30��)
		//HEART_X job ��ť
		ToDo* job = chatserver->_todopool.Alloc();
		job->type = HEART_O;
		job->packet = nullptr;

		chatserver->_todoQ.Enqueue(job);
		chatserver->_todoEvent.notify_one();

	}
}

unsigned __stdcall ChatServer::MonitorThread(LPVOID arg)
{
	ChatServer* chatserver = (ChatServer*)arg;
	//std::chrono::steady_clock::time_point timecheck;
	//timecheck = std::chrono::steady_clock::now();
	while (1)
	{

		std::this_thread::sleep_for(std::chrono::seconds(1));


		chatserver->_acceptTPS = chatserver->_acceptcount;
		chatserver->_logicTPS = chatserver->_logiccount;
		chatserver->_MLoginTPS = chatserver->_logincount;
		chatserver->_MSectorTPS = chatserver->_sectorcount;
		chatserver->_MMessageTPS = chatserver->_messagecount;
		chatserver->_MHeartbeatTPS = chatserver->_heartcount;

		InterlockedExchange(&chatserver->_acceptcount, 0);
		InterlockedExchange(&chatserver->_logiccount, 0);
		InterlockedExchange(&chatserver->_logincount, 0);
		InterlockedExchange(&chatserver->_sectorcount, 0);
		InterlockedExchange(&chatserver->_messagecount, 0);
		InterlockedExchange(&chatserver->_heartcount, 0);

	}
}

void  ChatServer::ProcAccept(ToDo* job)
{

	if (job->sessionId == errorsession)
	{
		DebugBreak();
	}

	ChatUser* user = _userpool.Alloc();
	user->clientaddr = job->clientaddr;
	user->sectorX = -1;
	user->sectorY = -1;
	user->sessionId = job->sessionId;
	user->heartbeat = std::chrono::steady_clock::now();


	_UnLoginMap.insert(std::make_pair(user->sessionId, user));

	InterlockedIncrement(&_acceptcount);

}

void ChatServer::ProcMessage(ToDo* job)
{
	CPacket packet = CPacket::GetPacketPtr(job->packet);
	WORD type;
	packet >> type;
	switch (type)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
	{
		Proc_REQ_LOGIN(job->sessionId,packet);
		break;
	}
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
	{
		Proc_REQ_SECTOR_MOVE(job->sessionId, packet);
		break;
	}
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
	{
		Proc_REQ_MESSAGE(job->sessionId, packet);
		break;
	}
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
	{
		Proc_REQ_HEARTBEAT(job->sessionId, packet);
		break;
	}
	default:
	{
		break;
	}
	}

}

void ChatServer::ProcRelease(ToDo* job)
{
	ChatUser* user;

	std::unordered_map<INT64, ChatUser*>::iterator it = _ChatUserMap.find(job->sessionId);
	//_ChatUserMap�� ����
	if (it == _ChatUserMap.end())
	{
		std::unordered_map<INT64, ChatUser*>::iterator uit = _UnLoginMap.find(job->sessionId);
		if (uit != _UnLoginMap.end())
		{
			user = uit->second;
			_UnLoginMap.erase(job->sessionId);
			_userpool.Free(user);
		}
		return;
	}
	user = it->second;
	//���Ϳ��� �����
	if (user->sectorX != -1 && user->sectorY != -1)
	{
		std::list<ChatUser*>::iterator dit = _Sector[user->sectorY][user->sectorX].begin();
		for (; dit != _Sector[user->sectorY][user->sectorX].end(); dit++)
		{
			ChatUser* deleteuser = *dit;
			if (deleteuser->accountNo == user->accountNo)
			{
				_Sector[user->sectorY][user->sectorX].erase(dit);
				break;
			}
		}
	}
	//���հ������� �����
	_ChatUserMap.erase(job->sessionId);
	_userpool.Free(user);
}

void ChatServer::ProcHeartO(ToDo* job)
{
	if (!_ChatUserMap.empty())
	{
		std::unordered_map<__int64, ChatUser*>::iterator it = _ChatUserMap.begin();
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		for (; it != _ChatUserMap.end(); it++)
		{
			ChatUser* user = it->second;
			std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(now - user->heartbeat);
			if (duration > std::chrono::seconds(40))
			{
				Disconnect(user->sessionId);
			}
		}
	}
}

void ChatServer::ProcHeartX(ToDo* job)
{
	if (!_UnLoginMap.empty())
	{
		std::unordered_map<__int64, ChatUser*>::iterator it = _UnLoginMap.begin();
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		for (; it != _UnLoginMap.end(); it++)
		{
			ChatUser* user = it->second;
			std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(now - user->heartbeat);
			if (duration > std::chrono::seconds(3))
			{
				Disconnect(user->sessionId);
			}
		}
	}
}


void ChatServer::Proc_REQ_LOGIN(__int64 sessionId, CPacket packet)
{
	INT64	accountno;
	WCHAR	id[20];
	WCHAR	nickname[20];		
	char	sessionkey[64];		
	packet >> accountno;
	packet.GetData((char*)id, 20*2);
	packet.GetData((char*)nickname, 20*2);
	packet.GetData(sessionkey, 64);

	bool status=LoginProcess();
	std::unordered_map<INT64, ChatUser*>::iterator it = _UnLoginMap.find(sessionId);
	if (it == _UnLoginMap.end())
	{
		errorsession = sessionId;
		_loginerror++;
		InterlockedIncrement(&_logiccount);
		InterlockedIncrement(&_logincount);
		return;
	}
	ChatUser* user = it->second;
	if (status)
	{
	//�α��� ����
		user->accountNo = accountno;
		user->heartbeat = std::chrono::steady_clock::now();
		memcpy(user->id, id, 20 * 2);
		memcpy(user->nickname, nickname, 20 * 2);
		memcpy(user->sessionKey, sessionkey, 64);
		_ChatUserMap.insert(std::make_pair(user->sessionId, user));
		_UnLoginMap.erase(user->sessionId);

		CPacket resmsg;
		MP_RES_LOGIN(&resmsg, status, user->accountNo);
		SendPacket(user->sessionId, resmsg);
	}
	else
	{
		_UnLoginMap.erase(user->sessionId);
		CPacket packet;
		MP_RES_LOGIN(&packet, status, user->accountNo);
		SendPacket(user->sessionId, packet);
	}
	InterlockedIncrement(&_logiccount);
	InterlockedIncrement(&_logincount);
}

void ChatServer::MP_RES_LOGIN(CPacket* packet, BYTE status, INT64 accountno)
{
	WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
	*packet << type << status << accountno;

}

void ChatServer::Proc_REQ_SECTOR_MOVE(__int64 sessionId, CPacket packet)
{
	INT64	accountno;
	WORD	sectorX;
	WORD	sectorY;
	packet >> accountno >> sectorX >> sectorY;

	ChatUser* user = _ChatUserMap.find(sessionId)->second;
	user->heartbeat = std::chrono::steady_clock::now();
	if (user->accountNo != accountno)
	{
		InterlockedIncrement(&_logiccount);
		InterlockedIncrement(&_sectorcount);
		return;
	}
	if (user->sectorX == -1 && user->sectorY == -1)
	{
		user->sectorX = sectorX;
		user->sectorY = sectorY;

		_Sector[user->sectorY][user->sectorX].push_back(user);
	}
	else
	{
		_Sector[user->sectorY][user->sectorX].remove(user);
		user->sectorX = sectorX;
		user->sectorY = sectorY; 
		_Sector[user->sectorY][user->sectorX].push_back(user);
	}

	CPacket resmsg;
	MP_RES_SECTOR_MOVE(&resmsg,accountno,sectorX,sectorY);
	SendPacket(user->sessionId, resmsg);
	InterlockedIncrement(&_logiccount);
	InterlockedIncrement(&_sectorcount);
}

void ChatServer::MP_RES_SECTOR_MOVE(CPacket* packet,INT64 accontno,WORD sectorx,WORD sectory)
{
	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*packet << type << accontno << sectorx << sectory;
}

void ChatServer::Proc_REQ_MESSAGE(__int64 sessionId, CPacket packet)
{
	INT64	accountno;
	WORD	len;
	packet >> accountno >> len;
	WCHAR* msg = new WCHAR[len / 2];
	packet.GetData((char*)msg, len);

	ChatUser* user = _ChatUserMap.find(sessionId)->second;
	user->heartbeat = std::chrono::steady_clock::now();
	CPacket resmsg;
	MP_RES_MESSAGE(&resmsg, accountno, user->id, user->nickname, len, msg);
	
	if (user->accountNo != accountno)
	{
		InterlockedIncrement(&_logiccount);
		InterlockedIncrement(&_messagecount);
		return;
	}


	//SendPacket(user->sessionId, resmsg);
	///*
	SectorAround around;
	GetSectorAround(user->sectorX, user->sectorY, &around);
	for (int i = 0; i < around.count; i++)
	{
		std::list<ChatUser*>::iterator mit = _Sector[around.around[i][1]][around.around[i][0]].begin();
		for (; mit != _Sector[around.around[i][1]][around.around[i][0]].end(); mit++)
		{
			ChatUser* tgt = *mit;
			SendPacket(tgt->sessionId, resmsg);
		}

	}
	//*/
	InterlockedIncrement(&_logiccount);
	InterlockedIncrement(&_messagecount);
}

void ChatServer::MP_RES_MESSAGE(CPacket* packet,INT64 accountno,WCHAR id[],WCHAR nickname[],WORD len,WCHAR msg[])
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
		//
		//		INT64	AccountNo
		//		WCHAR	ID[20]						// null ����
		//		WCHAR	Nickname[20]				// null ����
		//		
		//		WORD	MessageLen
		//		WCHAR	Message[MessageLen / 2]		// null ������
	*packet << type << accountno;
	packet->PutData(reinterpret_cast<char*>(id), 20*2);
	packet->PutData(reinterpret_cast<char*>(nickname), 20*2);
	*packet << len;
	packet->PutData(reinterpret_cast<char*>(msg), len);
}

void ChatServer::Proc_REQ_HEARTBEAT(__int64 sessionId, CPacket packet)
{
	ChatUser* user = _ChatUserMap.find(sessionId)->second;
	user->heartbeat = std::chrono::steady_clock::now();
	InterlockedIncrement(&_logiccount);
	InterlockedIncrement(&_heartcount);
}

bool ChatServer::LoginProcess()
{
	return true;
}

void ChatServer::GetSectorAround(int SectorX, int SectorY, SectorAround* sectoraround)
{
		sectoraround->count = 0;
		if (SectorX > 0)
		{
			//��
			if (SectorY > 0)
			{
				sectoraround->around[sectoraround->count][0] = SectorX - 1;
				sectoraround->around[sectoraround->count][1] = SectorY - 1;
				sectoraround->count++;
			}

			//��
			if (SectorY < SECTOR_MAX_Y - 1)
			{
				sectoraround->around[sectoraround->count][0] = SectorX - 1;
				sectoraround->around[sectoraround->count][1]= SectorY + 1;
				sectoraround->count++;
			}

			//?
			sectoraround->around[sectoraround->count][0] = SectorX - 1;
			sectoraround->around[sectoraround->count][1] = SectorY;
			sectoraround->count++;
		}

		//?
		if (SectorY > 0)
		{
			sectoraround->around[sectoraround->count][0] = SectorX;
			sectoraround->around[sectoraround->count][1] = SectorY - 1;
			sectoraround->count++;
		}
		//?
		if (SectorY < SECTOR_MAX_Y - 1)
		{
			sectoraround->around[sectoraround->count][0] = SectorX;
			sectoraround->around[sectoraround->count][1] = SectorY + 1;
			sectoraround->count++;
		}

		if (SectorX < SECTOR_MAX_X - 1)
		{
			//��
			if (SectorY > 0)
			{
				sectoraround->around[sectoraround->count][0] = SectorX + 1;
				sectoraround->around[sectoraround->count][1] = SectorY - 1;
				sectoraround->count++;
			}

			//��
			if (SectorY < SECTOR_MAX_Y - 1)
			{
				sectoraround->around[sectoraround->count][0] = SectorX + 1;
				sectoraround->around[sectoraround->count][1] = SectorY + 1;
				sectoraround->count++;
			}

			//?
			sectoraround->around[sectoraround->count][0] = SectorX + 1;
			sectoraround->around[sectoraround->count][1] = SectorY;
			sectoraround->count++;
		}

		//������ �ִ� ����
		sectoraround->around[sectoraround->count][0] = SectorX;
		sectoraround->around[sectoraround->count][1] = SectorY;
		sectoraround->count++;



}
