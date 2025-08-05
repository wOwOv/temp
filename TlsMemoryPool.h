#ifndef  __TLSMEMORYPOOL__
#define  __TLSMEMORYPOOL__
#include <new.h>
#include <windows.h>

static unsigned long Cookie = 0x01010100;

template <class DATA>
class TlsMemoryPool
{
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;

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

	TlsMemoryPool(int BlockNum=100, int bunchsize=100,bool PlacementNew = false, bool maxflag = false)
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

		_top = nullptr;
		_bunchCount=0;
		_key = 0;

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

			//������ ȣ���� ���·� ������
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
			else//������ ȣ�� ���� ������
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
			tpool->_storedCount = tpool->_nodeCount+tpool->_freeCount;

			TlsSetValue(_tlsIndex, (LPVOID)tpool);
		}

		//����Ϸ��� �����ϰ� �ִ� �� ��尡 �ִٸ� 
		if (tpool->_nodeCount > 0)
		{
			allocated = tpool->_nodelist;
			tpool->_nodelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if (_pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_nodeCount--;
		}
		//�� ���� ���� ��ȯ�Ϸ��� �����ص� ��尡 �ִٸ�
		else if (tpool->_freeCount > 0)
		{
			allocated = tpool->_freelist;
			tpool->_freelist = allocated->_next;
			//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
			if ( _pnFlag != 0)
			{
				new(&(allocated->_data)) DATA;
			}
			tpool->_freeCount--;
		}
		//����Ǯ���� �޾ƿ;��Ѵٸ�
		else
		{
			Node* nodebunch = GetBucket();
			//����Ǯ���� ��幭�� �޾ƿ�
			if (nodebunch != nullptr)
			{
				//�� ��帮��Ʈ�� ����
				tpool->_nodelist = nodebunch;

				//��� ���� �ֱ�
				allocated = tpool->_nodelist;
				tpool->_nodelist = allocated->_next;
				//_pnFlag�� 1�̸� ������ ȣ���ؼ� ��������
				if ( _pnFlag != 0)
				{
					new(&(allocated->_data)) DATA;
				}
				tpool->_nodeCount =  _bunchSize - 1;
			}
			//����Ǯ������ �� �޾ƿ���
			else
			{
				//��¥ �Ҵ��ؼ� �����
				allocated = new Node;
				allocated->_underguard =  _cookie;
				allocated->_overguard = _cookie;
			}
			
		}
		tpool->_storedCount = tpool->_nodeCount + tpool->_freeCount;

		InterlockedIncrement(&_usingCount);

		return &(allocated->_data);
	}




	//////////////////////////////////////////////////////////////////////////
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////��ȯ�� Data*��ó�� Node�Ҵ� �ּҸ� ã�Ƽ� �װ� node�鿡 �����־������.
	bool Free(DATA* data)
	{
		//����Ҵ� �ּ� ã��
		char* address = (char*)data;
		address -=  _position;
		Node* retnode = (Node*)address;

		//���.�����÷ο� Ȯ�� �� �´� ��Ұ� ���Դ��� Ȯ��
		if (retnode->_underguard !=  _cookie || retnode->_overguard !=  _cookie)
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

			//������ ȣ���� ���·� ������
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
			else//������ ȣ�� ���� ������
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

		//_pnFlag�� 1�̶�� �Ҹ��� ȣ���ؼ� ����
		if ( _pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		//nodelist �ڸ��� �ִٸ�
		if (tpool->_nodeCount <  _bunchSize)
		{
			retnode->_next = tpool->_nodelist;
			tpool->_nodelist = retnode;
			tpool->_nodeCount++;
		}
		//nodelist�� �ڸ��� ���ٸ� freelist��
		else
		{
			retnode->_next = tpool->_freelist;
			tpool->_freelist = retnode;
			tpool->_freeCount++;

			//freelist�� �� á�ٸ�
			if (tpool->_freeCount ==  _bunchSize)
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
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
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

//Bucket ���ÿ��� ���� ��ȯ�� �� ���� �Լ�
private:
	void ReturnBucket(Node* nodebunch)
	{
		Bunch* bunch = (Bunch*)nodebunch;
		unsigned long long tagadr = (unsigned long long)bunch;
		unsigned long long tag = InterlockedIncrement16(&_key);
		tagadr |= (tag << 48);
		Bunch* tagbucket = (Bunch*)tagadr;
		Bunch* oldtop;
		do
		{
			oldtop = _top;
			bunch->_bunchnext = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagbucket, (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_bunchCount);
	}
	Node* GetBucket()
	{
		Bunch* oldtop;
		Bunch* newtop;
		Bunch* realadr;
		unsigned long long tempadr;
		Node* retptr;
		long size = InterlockedDecrement(&_bunchCount);
		if (size < 0)
		{
			InterlockedIncrement(&_bunchCount);
			return nullptr;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr &= ADRMASK;
			realadr = (Bunch*)tempadr;
			newtop = realadr->_bunchnext;
			retptr = realadr;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

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
	
	Bunch* _top;							//Bunch Top
	unsigned long _bunchCount;				//���� ���� Bunch����
	short _key;								//tag
};




#endif

