#include "ChatServer.h"
#include <process.h>
#include <unordered_map>
#include <list>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>



ChatServer::ChatServer() : CoreServer(NET)
{
	//_LogicThread= (HANDLE)_beginthreadex(NULL, 0, &LogicThread, this, 0, NULL);
	_LogicThread=std::thread(LogicThread,this);
	//_TimerThread= (HANDLE)_beginthreadex(NULL, 0, &TimerThread, this, 0, NULL);
	_TimerThread = std::thread(TimerThread, this);
	//_todoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//_LogicThread.join();
	//_TimerThread.join();
}

ChatServer::~ChatServer()
{
}

void ChatServer::PrintAcceptCall()
{
	printf("%d\n", _acceptcall);
}


bool ChatServer::OnConnectionRequest(SOCKADDR_IN* clientaddr)
{
	return false;
}

void ChatServer::OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID)
{
	InterlockedIncrement(&_acceptcall);
	//ACCEPT job 인큐
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
	//MESSAGE job 인큐
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
	//ACCEPT job 인큐
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

		//로그인요청 안한 유저들 끊기(3초)
		for (int i = 0; i < 10; i++)
		{
			//Sleep(chatserver->_timeout);
			std::this_thread::sleep_for(std::chrono::seconds(3));
			//HEART_X job 인큐
			ToDo* job = chatserver->_todopool.Alloc();
			job->type = HEART_X;
			job->packet = nullptr;

			chatserver->_todoQ.Enqueue(job);
			chatserver->_todoEvent.notify_one();
			//SetEvent(chatserver->_todoEvent);
		}

		//로그인 한 비정상적인 유저들 끊기(30초)
		//HEART_X job 인큐
		ToDo* job = chatserver->_todopool.Alloc();
		job->type = HEART_O;
		job->packet = nullptr;

		chatserver->_todoQ.Enqueue(job);
		chatserver->_todoEvent.notify_one();

	}
}

void  ChatServer::ProcAccept(ToDo* job)
{
	ChatUser* user = _userpool.Alloc();
	user->clientaddr = job->clientaddr;
	user->sectorX = -1;
	user->sectorY = -1;
	user->sessionId = job->sessionId;
	user->heartbeat = std::chrono::steady_clock::now();

	_UnLoginMap.insert(std::make_pair(user->sessionId, user));
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
	ChatUser* user = _ChatUserMap.find(job->sessionId)->second;
	//_ChatUserMap에 없음
	if (user == _ChatUserMap.end()->second)
	{
		user = _UnLoginMap.find(job->sessionId)->second;
		if (user != _UnLoginMap.end()->second)
		{
			_UnLoginMap.erase(job->sessionId);
			_userpool.Free(user);
		}
		return;
	}
	//섹터에서 지우기
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
	//통합관리에서 지우기
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
	ChatUser* user = _UnLoginMap.find(sessionId)->second;
	if (status)
	{
	//로그인 성공
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


}

void ChatServer::MP_RES_MESSAGE(CPacket* packet,INT64 accountno,WCHAR id[],WCHAR nickname[],WORD len,WCHAR msg[])
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
		//
		//		INT64	AccountNo
		//		WCHAR	ID[20]						// null 포함
		//		WCHAR	Nickname[20]				// null 포함
		//		
		//		WORD	MessageLen
		//		WCHAR	Message[MessageLen / 2]		// null 미포함
	*packet << type << accountno;
	packet->PutData((char*)id, 20*2);
	packet->PutData((char*)nickname, 20*2);
	*packet << len;
	packet->PutData((char*)msg, len);
}

void ChatServer::Proc_REQ_HEARTBEAT(__int64 sessionId, CPacket packet)
{
	ChatUser* user = _ChatUserMap.find(sessionId)->second;
	user->heartbeat = std::chrono::steady_clock::now();
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
			//↖
			if (SectorY > 0)
			{
				sectoraround->around[sectoraround->count][0] = SectorX - 1;
				sectoraround->around[sectoraround->count][1] = SectorY - 1;
				sectoraround->count++;
			}

			//↙
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
			//↗
			if (SectorY > 0)
			{
				sectoraround->around[sectoraround->count][0] = SectorX + 1;
				sectoraround->around[sectoraround->count][1] = SectorY - 1;
				sectoraround->count++;
			}

			//↘
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

		//본인이 있는 섹터
		sectoraround->around[sectoraround->count][0] = SectorX;
		sectoraround->around[sectoraround->count][1] = SectorY;
		sectoraround->count++;



}
