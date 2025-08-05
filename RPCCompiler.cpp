/*
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

//대문자로 변환하는 함수
void to_upper(char* p);

int main(int argc, char* argv[])
{
	//char filename[128];
	//scanf("%s", filename);
	//argc = 2;

	for (int i = 1; i < argc; i++)
	{
		//stub 파일 생성용 
		// 
			//FILE* file = fopen(filename, "rb");
		{
			FILE* file = fopen(argv[i], "rb");
		if (file == NULL)
		{
			printf("fopen error\n");
		}

		//버퍼에 통으로 읽어오기
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		char* buffer = new char[size];
		rewind(file);
		int error = fread(buffer, size, 1, file);
		if (error == 0)
		{
			printf("fread error\n");
		}
		fclose(file);

		//생성할 파일 이름 뜯어오기
		char* ptr = buffer;
		char stubheadername[128];
		char stubclassname[128];
		int cnt = 0;
		while (*ptr != 0x0d)
		{
			stubheadername[cnt] = *ptr;
			stubclassname[cnt] = *ptr;
			cnt++;
			ptr++;
		}

		//헤더파일
		strcpy(&stubheadername[cnt], "Stub.h");
		strcpy(&stubclassname[cnt], "Stub");

		ptr += 5;

		char CSfunc[256][128];
		char deffunc[256][128];
		char defnum[256][4];
		int defnumber[256];

		int f = 0;
		int c = 0;
		while (1)
		{
			if (*ptr == 'S')
			{
				do
				{
					ptr++;
				} while (*ptr != 0x0d);
				ptr += 2;
			}
			if (*ptr == 'C')
			{
				ptr += 3;
				while (1)
				{
					CSfunc[f][c] = *ptr;
					deffunc[f][c] = *ptr;
					c++;
					ptr++;

					if (*ptr == '(')
					{
						CSfunc[f][c] = '\0';
						deffunc[f][c] = '\0';
						do
						{
							ptr++;
						} while (*ptr != ';');
						ptr += 2;
						int n = 0;
						do
						{
							defnum[f][n] = *ptr;
							ptr++;
							n++;
						} while (*ptr != 0x0d);
						defnum[f][n] = '\0';
						ptr += 2;
						break;
					}
				}
			}

			f++;

			if (*ptr == '}')
			{
				break;
			}
		}

		for (int i = 0; i < f; i++)
		{
			to_upper(deffunc[i]);
		}

		for (int i = 0; i < f; i++)
		{
			defnumber[i] = atoi(defnum[i]);
		}







		//Stub.h 작성
		FILE* stub = fopen(stubheadername, "wb");
		fprintf(stub, "#pragma once\n\n");
		fprintf(stub, "#include \"CoreServer.h\"\n");
		fprintf(stub, "#include \"CPacket.h\"\n\n");
		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "#define %s %d\n", deffunc[i], defnumber[i]);
		}
		fprintf(stub, "\n\n");
		fprintf(stub, "class %s:IStub\n{\n", stubclassname);
		fprintf(stub, "void ProcMessage(__int64 sessionID, CPacket packet)\n{\n");
		fprintf(stub, "unsigned char code;\n");
		fprintf(stub, "unsigned char size;\n");
		fprintf(stub, "unsigned char type;\n");
		fprintf(stub, "packet>>code>>size>>type;\n");
		fprintf(stub, "switch(type)\n{\n");
		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "case %s:\n{\n", deffunc[i]);
			fprintf(stub, "Proc%s(sessionID,packet);\n", CSfunc[i]);
			fprintf(stub, "break;\n}\n");
		}
		fprintf(stub, "default:\n{\nbreak;\n}\n");
		fprintf(stub, "}\n}\n\n\n");

		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "virtual void Proc%s(sessionID,packet);\n", CSfunc[i]);
		}

		fprintf(stub, "};\n\n");

		fclose(stub);
	}



	//proxy 파일 생성용 
	//
		//FILE* file = fopen(filename, "rb");
		FILE* file = fopen(argv[i], "rb");
		if (file == NULL)
		{
			printf("fopen error\n");
		}

		//버퍼에 통으로 읽어오기
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		char* buffer = new char[size];
		rewind(file);
		int error = fread(buffer, size, 1, file);
		if (error == 0)
		{
			printf("fread error\n");
		}
		fclose(file);

		//생성할 파일 이름 뜯어오기
		char* ptr = buffer;
		char proxyheadername[128];
		char proxycppname[128];
		
		int cnt = 0;
		while (*ptr != 0x0d)
		{
			proxyheadername[cnt] = *ptr;
			proxycppname[cnt] = *ptr;
			cnt++;
			ptr++;
		}

		//헤더파일
		strcpy(&proxyheadername[cnt], "Proxy.h");
		strcpy(&proxycppname[cnt], "Proxy.cpp");

		ptr += 5;

		char SCfunc[256][128];
		char deffunc[256][128];
		char defnum[256][4];
		int defnumber[256];

		int f = 0;
		int c = 0;
		while (1)
		{
			if (*ptr == 'S')
			{
				do
				{
					ptr++;
				} while (*ptr != 0x0d);
				ptr += 2;
			}
			if (*ptr == 'C')
			{
				ptr += 3;
				while (1)
				{
					CSfunc[f][c] = *ptr;
					deffunc[f][c] = *ptr;
					c++;
					ptr++;

					if (*ptr == '(')
					{
						CSfunc[f][c] = '\0';
						deffunc[f][c] = '\0';
						do
						{
							ptr++;
						} while (*ptr != ';');
						ptr += 2;
						int n = 0;
						do
						{
							defnum[f][n] = *ptr;
							ptr++;
							n++;
						} while (*ptr != 0x0d);
						defnum[f][n] = '\0';
						ptr += 2;
						break;
					}
				}
			}

			f++;

			if (*ptr == '}')
			{
				break;
			}
		}

		for (int i = 0; i < f; i++)
		{
			to_upper(deffunc[i]);
		}

		for (int i = 0; i < f; i++)
		{
			defnumber[i] = atoi(defnum[i]);
		}







		//Stub.h 작성
		FILE* stub = fopen(stubheadername, "wb");
		fprintf(stub, "#pragma once\n\n");
		fprintf(stub, "#include \"CoreServer.h\"\n");
		fprintf(stub, "#include \"CPacket.h\"\n\n");
		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "#define %s %d\n", deffunc[i], defnumber[i]);
		}
		fprintf(stub, "\n\n");
		fprintf(stub, "class %s:IStub\n{\n", stubclassname);
		fprintf(stub, "void ProcMessage(__int64 sessionID, CPacket packet)\n{\n");
		fprintf(stub, "unsigned char code;\n");
		fprintf(stub, "unsigned char size;\n");
		fprintf(stub, "unsigned char type;\n");
		fprintf(stub, "packet>>code>>size>>type;\n");
		fprintf(stub, "switch(type)\n{\n");
		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "case %s:\n{\n", deffunc[i]);
			fprintf(stub, "Proc%s(sessionID,packet);\n", CSfunc[i]);
			fprintf(stub, "break;\n}\n");
		}
		fprintf(stub, "default:\n{\nbreak;\n}\n");
		fprintf(stub, "}\n}\n\n\n");

		for (int i = 0; i < f; i++)
		{
			fprintf(stub, "virtual void Proc%s(sessionID,packet);\n", CSfunc[i]);
		}

		fprintf(stub, "};\n\n");

		fclose(stub);
	}



}

void to_upper(char* p)
{
	int count = 0;
	int diff = 'a' - 'A';
	while (p[count] != 0)
	{
		if (p[count] >= 97 && p[count] <= 122)
		{
			p[count] -= diff;
		}
		count++;
	}

}
*/