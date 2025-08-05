///*
#include <stdio.h>
#include "ChatServer.h"

int main()
{
	CoreServer* chatserver = new ChatServer();
	chatserver->Start("ChatServer_Config.txt", dfPACKET_CODE, dfPACKET_KEY);
	while (1)
	{
		Sleep(1000);
		ChatServer* temp = (ChatServer*)chatserver;
		temp->PrintAcceptCall();
		if (GetAsyncKeyState(0x46) & 0x8001)\
		{
			chatserver->GetIndex();
		}

	}

}
//*/