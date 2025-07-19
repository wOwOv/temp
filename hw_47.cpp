#include <iostream>
#include <process.h>
#include "CrashDump.h"
#include "LockFreeStack.h"
//#include "DCrash.h"
//#include "DStack.h"

CrashDump dumpit;


struct TestDATA
{
    int data;
    unsigned long count=0;
};


LFStack<TestDATA*> lfstack;

unsigned long flag = 3;

unsigned __stdcall WorkerThread0(LPVOID arg);
unsigned __stdcall WorkerThread1(LPVOID arg);
unsigned __stdcall WorkerThread2(LPVOID arg);
unsigned __stdcall WorkerThread3(LPVOID arg);

DWORD id0;
DWORD id1;
DWORD id2;
DWORD id3;

int main()
{
    HANDLE hThread[10];

    //hThread[9] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread2, NULL, 0, NULL);

    Sleep(3000);
    for (int i = 0; i < 2; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread0, NULL, 0, NULL);
        printf("thread%d start\n",i);
    }
    Sleep(1000);
    for (int i = 0; i < 2; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread1, NULL, 0, NULL);
        printf("thread%d start\n", i);
    }
    while (1)
    {
        //z키
        if (GetAsyncKeyState(0x5A) & 0x8001)
        {
            InterlockedExchange(&flag, 0);
        }
        //x키
        if (GetAsyncKeyState(0x58) & 0x8001)
        {
            InterlockedExchange(&flag, 1);
        }
        //c키
        if (GetAsyncKeyState(0x43) & 0x8001)
        {
            InterlockedExchange(&flag, 3);
        }
    }
}

unsigned int __stdcall WorkerThread0(LPVOID arg)
{
    while (1)
    {

            for (int i = 0; i < 100; i++)
            {
                TestDATA* data = new TestDATA;
                data->data = i;
                InterlockedIncrement(&data->count);
                lfstack.Push(data);
            }
            printf("After Push %lld\n", lfstack.GetSize());
            Sleep(1);

       
    }
    return 0;
}

unsigned int __stdcall WorkerThread1(LPVOID arg)
{
    while (1)
    {
        

            for (int i = 0; i < 100; i++)
            {
                TestDATA* data=nullptr;
                lfstack.Pop(&data);
                if (data != nullptr)
                {

                   free (data);
                }

            }
            printf("After Pop %lld\n", lfstack.GetSize());
           


    }
    return 0;
}


unsigned int __stdcall WorkerThread2(LPVOID arg)
{

    for (int i = 0; i < 500000; i++)
    {
        TestDATA* data = new TestDATA;
        data->data = i;
        InterlockedIncrement(&data->count);
        lfstack.Push(data);
    }

    for (int i = 0; i < 50000; i++)
    {
        TestDATA* data;
        lfstack.Pop(&data);
        if (data != nullptr)
        {
            if (InterlockedDecrement(&data->count) != 0)
            {
                DebugBreak();
            }
            free(data);
        }



        printf("setting end\n");


        return 0;
    }
}