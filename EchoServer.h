#ifndef __ECHOSERVER__
#define __ECHOSERVER__

#include "CoreServer.h"


class EchoServer :public CoreServer
{
public:
	EchoServer() :CoreServer(LAN)
	{
	}

private:
	virtual void OnMessage(__int64 sessionID, CPacket packet) override;
	virtual bool OnConnectionRequest(SOCKADDR_IN* clientaddr) override;
	virtual void OnAccept(SOCKADDR_IN* clientaddr, __int64 sessionID) override;
	virtual void OnRelease(__int64 sessionID) override;
};

#endif