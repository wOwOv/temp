#include "EchoServer.h"
#include "CPacket.h"

void EchoServer::OnMessage(__int64 sessionID, CPacket packet)
{
	CPacket msg;
	msg.Clear();
	int ret=msg.PutData(packet.GetReadPtr(), packet.GetDataSize());
	if (ret != packet.GetDataSize())
	{
		DebugBreak();
	}
	SendPacket(sessionID, msg);
}

bool EchoServer::OnConnectionRequest(SOCKADDR_IN* clientaddr)
{
	return false;
}

void EchoServer::OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID)
{
	__int64 data = 0x7fffffffffffffff;
	CPacket msg;
	msg.Clear();
	msg << data;
	SendPacket(sessionID, msg);
}

void EchoServer::OnRelease(__int64 sessionID)
{
}
