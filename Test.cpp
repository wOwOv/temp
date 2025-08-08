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
		//temp->PrintAcceptCall();
		if (GetAsyncKeyState(0x46) & 0x8001)
		{
			chatserver->GetIndex();
		}
		if (GetAsyncKeyState(0x53) & 0x8001)
		{
			temp->PrintSectorCount();
		}
		if (GetAsyncKeyState(0x45) & 0x8001)
		{
			temp->PrintLoginError();
		}
		printf("**********************************************************\n");
		printf("CoreServer Session: %d\n", chatserver->GetSessionCount());
		printf("SBufferPool: %d\n", chatserver->GetSBufferCapacity());

		printf("\nToDoPool: %d\n", temp->GetToDoPoolCapacity());
		printf("ToDoQ: %d\n", temp->GetToDoQueueRemained());

		printf("\nChatUserPool: %d\n", temp->GetChatUserPoolCapacity());
		printf("UnLogin: %d\n", temp->GetUnLoginCount());
		printf("ChatUser: %d\n", temp->GetChatUserCount());

		printf("\nAcceptTotal: %d\n", temp->GetAcceptTotal());
		printf("LogicTPS: %d\n", temp->GetLogicTPS());
		printf("LoginTPS: %d\n", temp->GetLoginTPS());
		printf("SectorTPS: %d\n",temp->GetSectorTPS());
		printf("MessageTPS: %d\n", temp->GetMessageTPS());
		printf("HeartTPS: %d\n", temp->GetHeartTPS());

		printf("\nRecvMesageTPS: %d\n", chatserver->GetRecvMessageTPS());
		printf("SendMessageTPS: %d\n", chatserver->GetSendMessageTPS());
		printf("**********************************************************\n");




	}

}
//*/