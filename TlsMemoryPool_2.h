#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>

static long cookie = 0x01010100;

template <class DATA>
class TlsMemoryPool
{
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;

	struct Node
	{
		Node* _retbucket;
		int _underguard;
		DATA _data;
		int _overguard;
	};

	struct Bucket
	{
		Node* _node = nullptr;
		unsigned long _max = 0;
		unsigned long _index = 0;
		unsigned long _returnCount = 0;
		Bucket* _next = nullptr;
	};


public:

	TlsMemoryPool(int bunchsize = 100, bool PlacementNew = false)
	{
		_tlsIndex = TlsAlloc();
		if (_tlsIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}
		Node* temp = new Node;
		_position = (long long)&temp->_data - (long long)&temp->_retbucket;
		_cookie = InterlockedIncrement(&cookie);

		_pnFlag = PlacementNew;

		_bunchsize = bunchsize;

	}
	~TlsMemoryPool()
	{
		TlsPool* tpool = (TlsPool*)TlsGetValue(_tlsIndex);
		if (tpool == nullptr)
		{
			DebugBreak();
		}



		TlsFree(_tlsIndex);
	}

	DATA* Alloc(void)
	{
		Node* allocated;

		Bucket* bucket = (Bucket*)TlsGetValue(_tlsIndex);

		if (bucket == nullptr)
		{
			//버켓 받은게 없으니 일단 공용풀 확인
			long size = InterlockedDecrement(&_num);
			if (size < 0)
			{
				InterlockedIncrement(&num);

				//내가 할당
				bucket = new Bucket;
				bucket->_max = _bunchsize;
				bucket->_node = (Node*)malloc(sizeof(Node) * _bunchsize);

				//생성자 호출한 상태로 보관

				for (int i = 0; i < _bunchsize; i++)
				{
					Node* node = &(bucket->_node[i]);
					node->_retbucket = bucket;
					if (_pnFlag == 0)
					{
						new(&bucket->_node[i]) DATA;
					}
				}

			}
			else
			{
				Bucket* oldtop;
				Bucket* newtop;
				Bucket* realadr;
				unsigned long long tempadr;
				do
				{
					tempadr = (unsigned long long)oldtop;
					tempadr &= ADRMASK;
					realadr = (Node*)tempadr;
					newtop = realadr->_next;
				} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

				bucket = realadr;
				bucket->_index = 0;
				bucket->_returnCount = 0;
			}

			TlsSetValue(_tlsIndex, (LPVOID)bucket);
		}

		unsigned long index = bucket->_index++;
		allocated = &(bucket->_node[index]);
		if (_pnFlag != 0)
		{
			new(&(bucket->_node[index])) DATA;
		}
		if (bucket->_index == bucket->_max)
		{
			TlsSetValue(_tlsIndex, nullptr);
		}

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

		//_pnFlag가 1이라면 소멸자 호출해서 보관
		if (_pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		Bucket* freebucket = retnode->_retbucket;
		if (InterlockedIncrement(&freebucket->_retunrCount) == _bunchsize)
		{
			//BucketPool에 반환
			unsigned long long tagbucket = (unsigned long long)freebucket;
			unsigned long long tag = InterlockedIncrement(&_key);
			tagbucket |= (tag << 48);
			Node* oldtop;
			do
			{
				oldtop = _top;
				freebucket->_next = oldtop;
			} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagbucket, (__int64)oldtop) != (__int64)oldtop);

			InterlockedDecrement(&_num);
		}

		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	long	GetCapacityCount(void)
	{
		return _num;
	}


private:
	int _cookie;
	bool _pnFlag;
	unsigned int _bunchsize;

	unsigned long long _position;

	DWORD _tlsIndex = 0;

	Bucket* _top;
	long _num;
	short _key;

};




#endif

