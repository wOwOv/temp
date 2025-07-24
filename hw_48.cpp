#include <iostream>
#include <process.h>
#include "LockFreeQueue.h"
#include "CrashDump.h"

#define REPEAT 10
CrashDump dumpit;

struct TestDATA
{
    int data;
    unsigned long count = 0;
};




unsigned __stdcall WorkerThread0(LPVOID arg);
unsigned __stdcall WorkerThread1(LPVOID arg);
unsigned __stdcall WorkerThread2(LPVOID arg);
unsigned __stdcall WorkerThread3(LPVOID arg);
unsigned __stdcall WorkerThread4(LPVOID arg);
unsigned __stdcall WorkerThread5(LPVOID arg);

extern unsigned long long logindex;

int main()
{
    LFQueue<TestDATA*> lfQ;
    LFQueue<TestDATA*> lfQ2;
    LFQueue<TestDATA*> lfQ3;

    HANDLE hThread[12];
    unsigned long long oldlogindex=10;
    for (int i = 0; i < 2; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread0, &lfQ, 0, NULL);
        printf("thread%d start\n", i);
    }
    for (int i = 2; i < 4; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread1, &lfQ, 0, NULL);
        printf("thread%d start\n", i);
    }
    for (int i = 4; i < 6; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread2, &lfQ2, 0, NULL);
        printf("thread%d start\n", i);
    }
    for (int i = 6; i < 8; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread3, &lfQ2, 0, NULL);
        printf("thread%d start\n", i);
    }
    for (int i = 8; i < 10; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread4, &lfQ3, 0, NULL);
        printf("thread%d start\n", i);
    }
    for (int i = 10; i < 12; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread5, &lfQ3, 0, NULL);
        printf("thread%d start\n", i);
    }
    while (1)
    {
        printf("%lld \n", logindex);
        if (oldlogindex == logindex)
        {
            //DebugBreak();
        }
        oldlogindex = logindex;
        Sleep(3000);
    }
}

unsigned __stdcall WorkerThread0(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ = (LFQueue<TestDATA*>*)arg;
    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {
        for (int i = 0; i < 500; i++)
        {
            TestDATA* data = new TestDATA;
            data->data = num++;
            data->count = 1;
            lfQ->Enqueue(data);
        }
       Sleep(1);
    }
    return 0;
}

unsigned __stdcall WorkerThread1(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ = (LFQueue<TestDATA*>*)arg;

    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {

        for (int i = 0; i < 500; i++)
        {
            TestDATA* data;
            bool res = lfQ->Dequeue(&data);
            if (res == false)
            {
                //DebugBreak();
            }
            else
            {
                if (data->count != 1)
                {
                    DebugBreak();
                }
                delete data;
            }

        }
    }
    return 0;
}

unsigned __stdcall WorkerThread2(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ2 = (LFQueue<TestDATA*>*)arg;

    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {
        for (int i = 0; i < 500; i++)
        {
            TestDATA* data = new TestDATA;
            data->data = num++;
            data->count = 2;
            lfQ2->Enqueue(data);
        }
        Sleep(1);
    }
    return 0;
}

unsigned __stdcall WorkerThread3(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ2 = (LFQueue<TestDATA*>*)arg;

    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {

        for (int i = 0; i < 500; i++)
        {
            TestDATA* data;
            bool res = lfQ2->Dequeue(&data);
            if (res == false)
            {
                //DebugBreak();
            }
            else
            {
                if (data->count != 2)
                {
                    DebugBreak();
                }
                delete data;
            }
        }
    }
    return 0;
}

unsigned __stdcall WorkerThread4(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ3 = (LFQueue<TestDATA*>*)arg;
    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {
        for (int i = 0; i < 500; i++)
        {
            TestDATA* data = new TestDATA;
            data->data = num++;
            data->count = 3;
            lfQ3->Enqueue(data);
        }
        Sleep(1);
    }
    return 0;
}

unsigned __stdcall WorkerThread5(LPVOID arg)
{
    LFQueue<TestDATA*>* lfQ3 = (LFQueue<TestDATA*>*)arg;

    DWORD id = GetCurrentThreadId();
    int num = 0;
    while (1)
    {

        for (int i = 0; i < 500; i++)
        {
            TestDATA* data;
            bool res = lfQ3->Dequeue(&data);
            if (res == false)
            {
                //DebugBreak();
            }
            else
            {
                if (data->count != 3)
                {
                    DebugBreak();
                }
                delete data;
            }
        }
    }
    return 0;
}