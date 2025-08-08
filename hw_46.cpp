/*
#include <iostream>
#include "EchoServer.h"
#include "Profiler.h"
#include <conio.h>
#include "CrashDump.h"

int number = 0;

struct CountBox
{
	__int64 TotalCount = 0;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
	int Min[2] = { 0,0 };		// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
	int Max[2] = { 0,0 };			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])
	int Call = 0;;				// 누적 호출 횟수.
};
CrashDump dumpit;

void RecordData(CountBox* box, int count)
{

	//총 횟수 더하기
	box->TotalCount += count;

	//최소값이면 데이터 넣기
	if (box->Min[0] == 0) {
		box->Min[0] = count;
	}
	else if (box->Min[1] == 0) {
		box->Min[1] = count;
	}
	else {
		if (box->Min[0] > count) {
			box->Min[0] = count;
		}
		else if (box->Min[1] > count) {
			box->Min[1] = count;
		}
	}
	//최대값이면 데이터 넣기
	if (box->Max[0] < count) {
		box->Max[0] = count;
	}
	else if (box->Max[1] < count) {
		box->Max[1] = count;
	}

	box->Call++;
}

void CountDataSave(CountBox* BoxArray)
{
	char txtname[18] = "TPS_";
	_itoa_s(number, &txtname[4], sizeof(txtname), 10);
	strcat_s(txtname, sizeof(txtname), ".txt");
	FILE* txtfile;
	fopen_s(&txtfile, txtname, "wt");
	if (txtfile != 0) {
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		fprintf(txtfile, "       Name      |     Average    |      Min     |      Max     |      Call            \n");
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		//AcceptTPS fprint
		float acceptavg = static_cast<float>(BoxArray[0].TotalCount - BoxArray[0].Max[0] - BoxArray[0].Max[1] - BoxArray[0].Min[0] - BoxArray[0].Min[1]) / (BoxArray[0].Call-4);
		fprintf(txtfile, "    AcceptTPS    |%14.4f|%12.4f|%12.4f|%14d\n",
			acceptavg, static_cast<float>(BoxArray[0].Min[0] + BoxArray[0].Min[1]) / 2,
			static_cast<float>(BoxArray[0].Max[0] + BoxArray[0].Max[1]) / 2,
			BoxArray[0].Call);
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		//RecvMessageTPS fprint
		float recvavg = static_cast<float>(BoxArray[1].TotalCount - BoxArray[1].Max[0] - BoxArray[1].Max[1] - BoxArray[1].Min[0] - BoxArray[1].Min[1]) / (BoxArray[1].Call-4);
		fprintf(txtfile, "     RecvTPS     |%14.4f|%12.4f|%12.4f|%14d\n",
			recvavg, static_cast<float>(BoxArray[1].Min[0] + BoxArray[1].Min[1]) / 2,
			static_cast<float>(BoxArray[1].Max[0] + BoxArray[1].Max[1]) / 2,
			BoxArray[1].Call);
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		//SendTPS fprint
		float sendavg = static_cast<float>(BoxArray[2].TotalCount - BoxArray[2].Max[0] - BoxArray[2].Max[1] - BoxArray[2].Min[0] - BoxArray[2].Min[1]) / (BoxArray[2].Call-4);
		fprintf(txtfile, "     SendTPS     |%14.4f|%12.4f|%12.4f|%14d\n",
			sendavg, static_cast<float>(BoxArray[2].Min[0] + BoxArray[2].Min[1]) / 2,
			static_cast<float>(BoxArray[2].Max[0] + BoxArray[2].Max[1]) / 2,
			BoxArray[2].Call);
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
	}

	fclose(txtfile);
	number++;
}


extern LONG64 LSENDPACKET;
extern unsigned long long Lspcall;

extern LONG64 LDOSEND;
extern LONG64 Lmsgcount;
extern unsigned long long Ldscall;

void DataSave()
{
	FILE* txt;
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	fopen_s(&txt, "PROCTime.txt", "wt");

	if (txt != 0)
	{
		fprintf(txt, "------------------------------------------------------------------------------------------------------\n");
		fprintf(txt, "       Name      |     Average    |      Call            |      Average          |      msgcount      \n");
		fprintf(txt, "------------------------------------------------------------------------------------------------------\n");
		fprintf(txt,"    SENDPACKET    |%14.4f|%14llu\n", (float)(LSENDPACKET / Freq.QuadPart * 1000000) / Lspcall,Lspcall);
		fprintf(txt,"      DOSEND      |%14.4f|%14llu|%14.4f|%14llu\n", (float)(LDOSEND / Freq.QuadPart * 1000000) / Ldscall, Ldscall, (float)(LDOSEND / Freq.QuadPart * 1000000) / Lmsgcount,Lmsgcount);
	}
	fclose(txt);
}


int main()
{
	int i = 0;
	CountBox countbox[3];
	CoreServer* echoserver=new EchoServer();
	echoserver->Start("LanServer_Config.txt");
	DWORD OldTick = timeGetTime();
	while (1)
	{
		if (_kbhit())
		{
			char temp = _getch();
		//	if (temp == 'e')
		//	{
		//		echoserver.Stop();
		//		return 0;
		//	}

			if (temp == 's')
			{
				CountDataSave(countbox);
				ProfileDataOutText();
				DataSave();

			}
		}
		if (i == 60 * 5)
		{
			CountDataSave(countbox);
			printf("TestOut\n");
		}
		if (i == 60 * 10)
		{
			CountDataSave(countbox);
			printf("TestOut\n");
		}
		if (i == 60 * 15)
		{
			ProfileDataOutText();
			CountDataSave(countbox);
			DataSave();
			printf("TestOut\n");
		}
		RecordData(&countbox[0], echoserver->getAcceptTPS());
		RecordData(&countbox[1], echoserver->getRecvMessageTPS());
		RecordData(&countbox[2], echoserver->getSendMessageTPS());
		i++;
		Sleep(1000);
	}
}
//*/