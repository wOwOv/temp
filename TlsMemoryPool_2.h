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
			//���� ������ ������ �ϴ� ����Ǯ Ȯ��
			long size = InterlockedDecrement(&_num);
			if (size < 0)
			{
				InterlockedIncrement(&num);

				//���� �Ҵ�
				bucket = new Bucket;
				bucket->_max = _bunchsize;
				bucket->_node = (Node*)malloc(sizeof(Node) * _bunchsize);

				//������ ȣ���� ���·� ����

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
	// ������̴� ���� �����Ѵ�.
	//
	// Parameters: (DATA *) �� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////��ȯ�� Data*��ó�� Node�Ҵ� �ּҸ� ã�Ƽ� �װ� node�鿡 �����־������.
	bool Free(DATA* data)
	{
		//����Ҵ� �ּ� ã��
		char* address = (char*)data;
		address -= _position;
		Node* retnode = (Node*)address;

		//���.�����÷ο� Ȯ�� �� �´� ��Ұ� ���Դ��� Ȯ��
		if (retnode->_underguard != _cookie || retnode->_overguard != _cookie)
		{
			return false;
		}

		//_pnFlag�� 1�̶�� �Ҹ��� ȣ���ؼ� ����
		if (_pnFlag != 0)
		{
			retnode->_data.~DATA();
		}

		Bucket* freebucket = retnode->_retbucket;
		if (InterlockedIncrement(&freebucket->_retunrCount) == _bunchsize)
		{
			//BucketPool�� ��ȯ
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
	// ���� Ȯ�� �� �� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
	//
	// Parameters: ����.
	// Return: (int) �޸� Ǯ ���� ��ü ����
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

