#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>

static unsigned long Cookie = 0x01010100;

#define MAX 5000000
unsigned long long Tlslogindex = 0;
struct TlsLogBox
{
	DWORD id;
	int type = 0xcccccccc;//0xbbbbbbbb Enqueue 0xdddddddd Dequeue
	int size = 0xcccccccc;
	int cookie = 0xcccccccc;
	void* curtail = nullptr;
	void* oldtail = nullptr;
	void* newtail = nullptr;
};

TlsLogBox Tlslogbox[MAX];

template <class DATA>
class TlsMemoryPool
{
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;
	const unsigned long long TAGMASK = 0xffff000000000000;
	const unsigned long long MAKETAG = 0x0001000000000000;

	struct Node
	{
		int _underguard;
		DATA _data;
		int _overguard;
		Node* _next;
		Node* _bunchnext;
	}typedef Bunch;

	struct TlsPool
	{
		Node* _nodelist;
		unsigned int _nodeCount;

		Node* _freelist;
		unsigned int _freeCount;

		int _storedCount;
	};

public:

	TlsMemoryPool(int BlockNum = 100, int bunchsize = 100, bool PlacementNew = false, bool maxflag = false)
	{
		_tlsIndex = TlsAlloc();
		if (_tlsIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}

		_cookie = InterlockedIncrement(&Cookie);

		Node* node = new Node;
		_position = (long long)&node->_data - (long long)&node->_underguard;
		delete node;

		_pnFlag = PlacementNew;
		_maxCount = BlockNum;
		_maxFlag = maxflag;

		_bunchSize = bunchsize;

		_usingCount = 0;

		//더미 bunch 생성

		_dummy = new Bunch;
		_dummy->_bunchnext = nullptr;
		_head = _dummy;
		_tail = _dummy;
		_bunchCount = 0;

	}
	~TlsMemoryPool()
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}

		Node* node = tpool->_nodelist;
		Node* temp = node->_next;
		while (1)
		{
			if (node != nullptr)
			{
				temp = node->_next;
				delete node;
				node = temp;
			}
			else
			{
				break;
			}
		}


		node = tpool->_freelist;
		temp = node->_next;
		while (1)
		{
			if (node != nullptr)
			{
				temp = node->_next;
				delete node;
				node = temp;
			}
			else
			{
				break;
			}
		}

		TlsFree(_tlsIndex);
	}

	DATA* Alloc(void)
	{
		Node* allocated;

		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			tpool = new TlsPool;
			tpool->_nodelist = nullptr;
			tpool->_nodeCount = 0;
			tpool->_freelist = nullptr;
			tpool->_freeCount = 0;
			tpool->_storedCount = 0;

			//생성자 호출한 상태로 들어가야함
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = new Node;
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//생성자 호출 없이 들어가야함
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxCount;
			tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//사용하려고 보관하고 있는 내 노드가 있다면 
		if (tpool->_nodeCount > 0)
		{
			allocated = tpool->_nodelist;
			tpool->_nodelist = allocated->_next;
			//_pnFlag가 1이면 생성자 호출해서 나가야함
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_nodeCount--;
		}
		//내 노드는 없고 반환하려고 보관해둔 노드가 있다면
		else if (tpool->_freeCount > 0)
		{
			allocated = tpool->_freelist;
			tpool->_freelist = allocated->_next;
			//_pnFlag가 1이면 생성자 호출해서 나가야함
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_freeCount--;
		}
		//공용풀에서 받아와야한다면
		else
		{
			Node* nodebunch = GetBucket();
			//공용풀에서 노드묶음 받아옴
			if (nodebunch != nullptr)
			{
				//내 노드리스트에 연결
				tpool->_nodelist = nodebunch;

				//노드 떼서 주기
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag가 1이면 생성자 호출해서 나가야함
				if (_pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount = _bunchSize - 1;
			}
			//공용풀에서도 못 받아왔음
			else
			{
				//진짜 할당해서 줘야함
				allocated = new Node;
				allocated->_underguard = _cookie;
				allocated->_overguard = _cookie;
			}

		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		InterlockedIncrement(&_usingCount);

		return &(allocated->_data);
	}




	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////반환된 Data*근처의 Node할당 주소를 찾아서 그걸 node들에 끼워넣어줘야함.
	bool Free(DATA* data)
	{
		//노드할당 주소 찾기
		char* address = (char*)data;
		address -= _position;
		Node* retnode = (Node*)address;

		//언더.오버플로우 확인 겸 맞는 요소가 들어왔는지 확인
		if (retnode->_underguard != _cookie || retnode->_overguard != _cookie)
		{
			return false;
		}

		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			tpool = new TlsPool;
			tpool->_nodelist = nullptr;
			tpool->_nodeCount = 0;
			tpool->_freelist = nullptr;
			tpool->_freeCount = 0;
			tpool->_storedCount = 0;

			//생성자 호출한 상태로 들어가야함
			if (_pnFlag == 0)
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = new Node;
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}
			else//생성자 호출 없이 들어가야함
			{
				for (int i = 0; i < _maxCount; i++)
				{
					Node* node = (Node*)malloc(sizeof(Node));
					node->_underguard = _cookie;
					node->_overguard = _cookie;

					node->_next = tpool->_nodelist;
					tpool->_nodelist = node;
				}
			}

			tpool->_nodeCount = _maxCount;
			tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//_pnFlag가 1이라면 소멸자 호출해서 보관
		if (_pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		//nodelist 자리가 있다면
		if (tpool->_nodeCount < _bunchSize)
		{
			retnode->_next = tpool->_nodelist;
			tpool->_nodelist = retnode;
			tpool->_nodeCount++;
		}
		//nodelist에 자리가 없다면 freelist로
		else
		{
			retnode->_next = tpool->_freelist;
			tpool->_freelist = retnode;
			tpool->_freeCount++;

			//freelist가 다 찼다면
			if (tpool->_freeCount == _bunchSize)
			{
				ReturnBucket(tpool->_freelist);
				tpool->_freelist = nullptr;
				tpool->_freeCount = 0;
			}
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		InterlockedDecrement(&_usingCount);

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int	GetStoredNodeCount(void)
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}
		return tpool->_storedCount;
	}

	unsigned long long GetBunchCount()
	{
		return _bunchCount;
	}

	unsigned long GetUsingCount()
	{
		return _usingCount;
	}

	//Bucket 스택에서 얻어보고 반환할 때 쓰는 함수
private:
	void ReturnBucket(Node* nodebunch)
	{
		nodebunch->_bunchnext = nullptr;
		unsigned long long tagbunch = (unsigned long long)nodebunch;

		Bunch* oldtail;
		Bunch* realadr;
		do
		{
			while (1)
			{
				oldtail = _tail;
				unsigned long long tag = (unsigned long long) oldtail;

				tag &= TAGMASK;
				tag += MAKETAG;
				tagbunch |= tag;

				unsigned long long tempadr = (unsigned long long)oldtail;
				tempadr &= ADRMASK;
				realadr = (Bunch*)tempadr;
				if (realadr->_bunchnext != nullptr)
				{
					InterlockedCompareExchange64((__int64*)& _tail, (__int64)realadr->_bunchnext, (__int64)oldtail);
				}
				else
				{
					break;
				}
			}
		} while (InterlockedCompareExchange64((__int64*)&realadr->_bunchnext, (__int64)tagbunch, (__int64)nullptr) != (__int64)nullptr);
		int qsize=InterlockedIncrement(&_bunchCount);
		__int64 curtail=InterlockedCompareExchange64((__int64*)&_tail, (__int64)tagbunch, (__int64)oldtail);


		unsigned long long index = InterlockedIncrement(&Tlslogindex);
		index %= MAX;
		Tlslogbox[index].id = GetCurrentThreadId();
		Tlslogbox[index].type = 0xbbbbbbbb;
		Tlslogbox[index].size = qsize;
		Tlslogbox[index].curtail = (Node*)curtail;
		Tlslogbox[index].oldtail = (Node*)oldtail;
		Tlslogbox[index].newtail = (Node*)tagbunch;

	}
	Node* GetBucket()
	{
		Bunch* oldhead;
		Bunch* newhead;
		Bunch* realadr;
		unsigned long long tempadr;
		unsigned long long tempadr2;

		Node* retptr;

		long size = InterlockedDecrement(&_bunchCount);
		if (size < 0)
		{
			InterlockedIncrement(&_bunchCount);
			return nullptr;
		}

		do
		{
			while (1)
			{
				Bunch* oldtail;
				Bunch* tailadr;

				oldtail = _tail;
				unsigned long long tempadr = (unsigned long long)oldtail;
				tempadr &= ADRMASK;
				tailadr = (Bunch*)tempadr;
				if (tailadr->_bunchnext == nullptr)
				{
					break;
				}
				InterlockedCompareExchange64((__int64*)&_tail, (__int64)tailadr->_bunchnext, (__int64)oldtail);
			}

			do
			{
				oldhead = _head;
				tempadr = (unsigned long long)oldhead;
				tempadr &= ADRMASK;
				realadr = (Bunch*)tempadr;
				newhead = realadr->_bunchnext;
			} while (newhead == nullptr);

			tempadr2 = (unsigned long long)newhead;
			tempadr2 &= ADRMASK;
			retptr = (Bunch*)tempadr2;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)newhead, (__int64)oldhead) != (__int64)oldhead);


		unsigned long long index = InterlockedIncrement(&Tlslogindex);
		index %= MAX;
		Tlslogbox[index].id = GetCurrentThreadId();
		Tlslogbox[index].type = 0xdddddddd;
		Tlslogbox[index].size = size;
		Tlslogbox[index].curtail = 0x0000000000000000;
		Tlslogbox[index].oldtail = (Node*)oldhead;
		Tlslogbox[index].newtail = (Node*)newhead;





		return retptr;
	}

private:
	int _cookie;
	long long _position;

	bool _pnFlag;
	int _maxCount;
	bool _maxFlag;

	unsigned int _bunchSize;

	unsigned long _usingCount;

	DWORD _tlsIndex = 0;

	Bunch* _head;
	Bunch* _tail;
	Bunch* _dummy;
	unsigned long _bunchCount;				//보관 중인 Bunch개수
};




#endif
