/*

	procademy MemoryPool.

	메모리 풀 클래스 (오브젝트 풀 / 프리리스트)
	특정 데이타(구조체,클래스,변수)를 일정량 할당 후 나눠쓴다.

	- 사용법.

	procademy::CMemoryPool<DATA> MemPool(300, FALSE);
	DATA *pData = MemPool.Alloc();

	pData 사용

	MemPool.Free(pData);


----------------------------------------------------------------*/
#ifndef  __MEMORYPOOL__
#define  __MEMORYPOOL__
#include <new.h>
#include <windows.h>

static int key = 0xaaaa;



template <class DATA>
class MemoryPool
{
private:
	struct NODE
	{
		int _underguard;
		DATA _data;
		int _overguard;
		NODE* _next;
	};


public:

	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 초기 블럭 개수.
	//				(bool) Alloc 시 생성자 / Free 시 파괴자 호출 여부
	// Return:
	//////////////////////////////////////////////////////////////////////////
	MemoryPool(int BlockNum, bool PlacementNew = false, bool maxflag = false)
	{
		_head = nullptr;
		_cookie = key;
		key++;
		_pnFlag = PlacementNew;

		_maxFlag = maxflag;

		_key = 0;

		NODE* node = new NODE;
		_position = (long long)&node->_data - (long long)&node->_underguard;
		delete node;

		//생성자 호출한 상태로 들어가야함
		if (_pnFlag == 0)
		{
			for (int i = 0; i < BlockNum; i++)
			{
				NODE* node = new NODE;
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _head;
				_head = node;
			}
		}
		else//생성자 호출 없이 들어가야함
		{
			for (int i = 0; i < BlockNum; i++)
			{
				NODE* node = (NODE*)malloc(sizeof(NODE));
				node->_underguard = _cookie;
				node->_overguard = _cookie;

				node->_next = _head;
				_head = node;
			}
		}


		_capacity = BlockNum;
		_usingCount = 0;


	}
	virtual	~MemoryPool()
	{
		NODE* node = _head;
		NODE* temp = node;
		//이미 생성자 호출되어있는 상태로 들어있음. 소멸자 호출 되어야함
		if (_pnFlag == 0)
		{
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
		}
		else
		{
			while (1)
			{
				if (node != nullptr)
				{
					temp = node->_next;
					free(node);
					node = temp;
				}
				else
				{
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.  
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA* Alloc(void)
	{
		NODE* allocated;
		NODE* oldhead;
		NODE* newhead;
		NODE* realadr;
		unsigned long long tempadr;
		do
		{
			oldhead = _head;
			if (oldhead == nullptr)
			{
				if (_maxFlag == true)
				{
					if (_maxcount == _capacity)
					{
						return nullptr;
					}
				}
				allocated = new NODE;
				allocated->_underguard = _cookie;
				allocated->_overguard = _cookie;
				realadr = allocated;
				InterlockedIncrement(&_capacity);
				break;
			}

			tempadr = (unsigned long long)oldhead;
			tempadr <<= 16;
			tempadr >>= 16;
			realadr = (NODE*)tempadr;
			newhead = realadr->_next;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)newhead, (__int64)oldhead) != (__int64)oldhead);
		allocated = realadr;
		//_pnFlag가 1이면 생성자 호출해서 나가야함
		if (_pnFlag != 0)
		{
			new(&(allocated->_data)) DATA;
		}


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
		NODE* retnode = (NODE*)address;

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

		unsigned long long temp = InterlockedIncrement16(&_key);
		unsigned long long countnode = (unsigned long long)retnode;
		countnode |= (temp << 48);
		NODE* oldhead;
		do
		{
			oldhead = _head;
			retnode->_next = oldhead;
		} while (InterlockedCompareExchange64((__int64*)&_head, (__int64)countnode, (__int64)oldhead) != (__int64)oldhead);


		InterlockedDecrement(&_usingCount);
		return true;
	}


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int	GetCapacityCount(void)
	{
		return _capacity;
	}

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int	GetUseCount(void)
	{
		return _usingCount;
	}


	void SetMaxCount(int maxcount)
	{
		_maxcount = maxcount;
	}

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.


private:
	NODE* _head;
	int _cookie;
	unsigned int _capacity;
	unsigned int _usingCount;
	bool _pnFlag;

	long long _position;

	int _maxcount;
	bool _maxFlag;

	short _key;
};












#endif
