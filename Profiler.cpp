#include <iostream>
#include <windows.h>
#include "Profiler.h"


struct stProfile{	
	long Flag;				// ���������� ��� ����. (�迭�ÿ���)
	char Name[64];			// �������� ���� �̸�.
	LARGE_INTEGER StartTime;			// �������� ���� ���� �ð�.
	__int64 TotalTime;			// ��ü ���ð� ī���� Time.	(��½� ȣ��ȸ���� ������ ��� ����)
	__int64 Min[2] = {-1,-1};			// �ּ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
	__int64 Max[2];			// �ִ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])
	__int64 Call;				// ���� ȣ�� Ƚ��.
	long flag; //1�̸� profilebegin, 0�̸� �� �� ��

};

stProfile PArr[20];
int txtnum;


//Profiling �����ϱ�
void ProfileBegin(const char* szName) {
	int idx = 0;
	for (; idx < 20; idx++) {
		if (PArr[idx].Flag) {
			if (PArr[idx].Call == 0) {
				throw 1;
				//DebugBreak();
			}
			if (strcmp(PArr[idx].Name, szName) == 0) {
				int check = InterlockedExchange(&PArr[idx].flag, 1);
				if (check != 0)
				{
					DebugBreak();
				}
				QueryPerformanceCounter(&PArr[idx].StartTime);
				break;
			}
		}
	}
	
	if (idx == 20) {
		for (idx = 0; idx < 20; idx++) {
			if (!PArr[idx].Flag) {
				PArr[idx].Flag = true;
				strcpy_s(PArr[idx].Name,sizeof(PArr[idx].Name), szName);
				QueryPerformanceCounter(&PArr[idx].StartTime);
				int check = InterlockedExchange(&PArr[idx].flag, 1);
				if (check != 0)
				{
					DebugBreak();
				}
				break;
			}
		}
	}
}


//Profiling ������
void ProfileEnd(const char* szName) {
	int idx = 0;
	for (; idx < 20; idx++) {
		if (PArr[idx].Flag) {
			if (strcmp(PArr[idx].Name, szName) == 0) {
				break;
			}
		}
	}
	if (idx == 20) {
		//throw 1;
		DebugBreak();
	}
	//�ѽð��� �ɸ� �ð� ���ϱ�
	LARGE_INTEGER End;
	QueryPerformanceCounter(&End);
	LARGE_INTEGER Time;
	Time.QuadPart= End.QuadPart - PArr[idx].StartTime.QuadPart;


	int Echeck = InterlockedExchange(&PArr[idx].flag, 0);
	if (Echeck != 1)
	{
		DebugBreak();
	}

	PArr[idx].TotalTime += Time.QuadPart;

	//�ּҰ��̸� ������ �ֱ�
	if(PArr[idx].Min[0]==-1){
		PArr[idx].Min[0] = Time.QuadPart;
	}
	else if (PArr[idx].Min[1] == -1) {
		PArr[idx].Min[1] = Time.QuadPart;
	}
	else {
		if (PArr[idx].Min[0] > Time.QuadPart) {
			PArr[idx].Min[0] = Time.QuadPart;
		}
		else if (PArr[idx].Min[1] > Time.QuadPart) {
			PArr[idx].Min[1] = Time.QuadPart;
		}
	}
	//�ִ밪�̸� ������ �ֱ�
	if (PArr[idx].Max[0] < Time.QuadPart) {
		PArr[idx].Max[0] = Time.QuadPart;
	}
	else if (PArr[idx].Max[1] < Time.QuadPart) {
		PArr[idx].Max[1] = Time.QuadPart;
	}

	PArr[idx].Call++;


}






//Profiling ������ txt�� ���
void ProfileDataOutText(void) {
	char txtname[18] = "Profiling_";
	_itoa_s(txtnum, &txtname[10],sizeof(txtname), 10);
	strcat_s(txtname, sizeof(txtname), ".txt");
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	FILE* txtfile;
	fopen_s(&txtfile,txtname, "wt");
	if (txtfile!=0) {
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		fprintf(txtfile, "       Name      |     Average    |      Min     |      Max     |      Call      \n");
		fprintf(txtfile, "----------------------------------------------------------------------------------\n");
		int idx = 0;
		for (; idx < 20; idx++) {
			if (PArr[idx].Flag) {
				float avg = static_cast<float>(PArr[idx].TotalTime - PArr[idx].Max[0] - PArr[idx].Max[1] - PArr[idx].Min[0] - PArr[idx].Min[1])/Freq.QuadPart *1000000/ (PArr[idx].Call - 4);
				fprintf(txtfile, "%17s|%14.4f��|%12.4f��|%12.4f��|%14d|%14d|%14d\n",
					PArr[idx].Name, avg, static_cast<float>(PArr[idx].Min[0] + PArr[idx].Min[1]) * 1000000 / 2/Freq.QuadPart ,
					static_cast<float>(PArr[idx].Max[0] + PArr[idx].Max[1]) * 1000000 / 2/ Freq.QuadPart,
					static_cast<int>(PArr[idx].Call));
			}
			if (!PArr[idx + 1].Flag) {
				fprintf(txtfile, "-------------------------------------------------------------------------------\n");
				break;
			}
		}

		fclose(txtfile);
		txtnum++;
	}
}




//�������ϸ��� ������ �ʱ�ȭ(�±� ����)
void ProfileReset(void) {
	for (int i = 0; i < 20; i++) {
		if (PArr[i].Flag) {
			PArr[i].Flag = false;
			PArr[i].StartTime.QuadPart = 0;
			PArr[i].TotalTime = 0;
			PArr[i].Min[0] = -1;
			PArr[i].Min[1] = -1;
			PArr[i].Max[0] = 0;
			PArr[i].Max[1] = 0;
			PArr[i].Call = 0;
		}
	}
}