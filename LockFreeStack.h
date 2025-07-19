#ifndef __LOCKFREESTACK__
#define __LOCKFREESTACK__
#include <windows.h>
#include <new.h>
#include "TlsMemoryPool.h"

//struct Box
//{
//	unsigned long long num=0;
//	int type = 0xcccccccc;//0 push 1 pop
//	int cookie = 0xcccccccc;
//	void* newnode = nullptr;
//	void* oldtop = nullptr;
//
//};
//#define MAX 30000000
//
//Box Box0[MAX];
//Box Box1[MAX];
//
//
//unsigned long long c0=0;
//unsigned long long c1=0;
//unsigned long long c2=0;
//unsigned long long c3=0;
//
//
//extern DWORD id0;
//extern DWORD id1;


template <class DATA>
class LFStack
{
	struct Node
	{
		DATA _data;
		Node* _next;
	};

public:
	LFStack() :_top(nullptr), _num(0)
	{
	}

	~LFStack()
	{
		if (_top != nullptr)
		{
			do
			{
				Node* temp = _top->_next;
				delete _top;
				_top = temp;

			} while (_top != nullptr);
		}
	}

	void Push(DATA data)
	{
		Node* newnode = _nodepool.Alloc();
		newnode->_data = data;
		unsigned long long temp = InterlockedIncrement16(&_key);
		unsigned long long countnode = (unsigned long long)newnode;
		countnode |= (temp << 48);
		Node* oldtop;
		do
		{
			oldtop = _top;
			newnode->_next = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)countnode , (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_num);
	}

	void Pop(DATA* data)
	{
		Node* oldtop;
		Node* newtop;
		Node* realadr;
		unsigned long long tempadr;
		int size = InterlockedDecrement(&_num);
		if (size < 0)
		{
			InterlockedIncrement(&_num);
			data = nullptr;
			return;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr <<= 16;
			tempadr >>= 16;
			realadr = (Node*)tempadr;
			newtop = realadr->_next;
			*data =realadr->_data;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);
		//InterlockedDecrement(&_num);

		_nodepool.Free(realadr);
	}

	unsigned long long GetSize()
	{
		return _num;
	}


private:
	Node* _top;
	unsigned long long _num;
	short _key= 0;
	inline static TlsMemoryPool<Node> _nodepool;
};

#endif