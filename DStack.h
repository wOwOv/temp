#pragma once
#pragma once
#include "Windows.h"
#include <new.h>

template <class DATA>
class LockFreeStack
{

public:

    struct Node
    {
        DATA data;
        Node* next;
    };

    struct LogData
    {
        unsigned int work;
        unsigned int thread_id;
        Node* before;
        Node* after;
        DATA data;
    };

    LockFreeStack() : top_node(NULL), log_index(0)
    {

    }


    virtual   ~LockFreeStack()
    {
        DATA temp;
        for (int i = 0; i < size; ++i)
        {

            Pop(&temp);
        }
    }

    //struct LogData
    //{
    //   unsigned int work;
    //   unsigned int thread_id;
    //   Node* before;
    //   Node* after;
    //   DATA data;
    //};

    bool Push(DATA new_data)
    {

        Node* new_node = new Node;
        new_node->data = new_data;
        unsigned int local_index;

        while (1)
        {
            new_node->next = top_node;
            if ((__int64)new_node->next == InterlockedCompareExchange64((__int64*)&top_node, (__int64)new_node, (__int64)new_node->next))
            {
                InterlockedIncrement(&size);
                local_index = InterlockedIncrement(&log_index);
                log_buffer[local_index].work = -1;
                log_buffer[local_index].thread_id = GetCurrentThreadId();
                log_buffer[local_index].before = new_node->next;
                log_buffer[local_index].after = new_node;
                log_buffer[local_index].data = new_data;

                break;
            }
        }



        return true;

    }


    bool Pop(DATA* out)
    {
        Node* local_top;
        unsigned int local_index;
        while (1)
        {
            local_top = top_node;
            //top_node = local_top->next;
            if ((__int64)local_top == InterlockedCompareExchange64((__int64*)&top_node, (__int64)local_top->next, (__int64)local_top))
            {

                *out = local_top->data;
                local_index = InterlockedIncrement(&log_index);
                log_buffer[local_index].work = 0;
                log_buffer[local_index].thread_id = GetCurrentThreadId();
                log_buffer[local_index].before = local_top;
                log_buffer[local_index].after = local_top->next;
                log_buffer[local_index].data = local_top->data;
                InterlockedDecrement(&size);
                break;
            }
        }

        delete local_top;

        return true;

    }


    int      GetSize(void) { return size; }


private:
    Node* top_node;
    long size;


    LogData log_buffer[10000000];
    unsigned int log_index;
};
