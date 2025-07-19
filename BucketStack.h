#ifndef __BUCKETSTACK__
#define __BUCKETSTACK__

#include"MemoryPool.h"
#include <windows.h>

static unsigned long commoncookie = 0x01010100;

//락프리스택
class BucketStack
{
	struct Bucket
	{
		void* _data;
		Bucket* _next;
	};
private:
	const unsigned long long ADRMASK = 0x0000ffffffffffff;

public:
	BucketStack() :_top(nullptr), _num(0), _key(0),_bucketpool(100)
	{
		_commoncookie = InterlockedIncrement(&commoncookie);
	}
	~BucketStack()
	{
		Bucket* bucket = _top;
		while (bucket != nullptr)
		{

			unsigned long long tempadr = (unsigned long long)bucket;
			tempadr &= ADRMASK;
			Bucket* realadr = (Bucket*)tempadr;
			Bucket* next = realadr->_next;
			delete realadr;

			bucket = next;
		}
	}

	void ReturnBucket(void* data)
	{

		Bucket* newnode = _bucketpool.Alloc();
		newnode->_data = data;
		
		unsigned long long tagnode = (unsigned long long)newnode;
		unsigned long long tag = InterlockedIncrement16(&_key);
		tagnode |= (tag << 48);
		Bucket* oldtop;
		do
		{
			oldtop = _top;
			newnode->_next = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagnode, (__int64)oldtop) != (__int64)oldtop);
		InterlockedIncrement(&_num);
	}

	void* GetBucket()
	{
		Bucket* oldtop;
		Bucket* newtop;
		Bucket* realadr;
		unsigned long long tempadr;
		void* retptr;
		long size = InterlockedDecrement(&_num);
		if (size < 0)
		{
			InterlockedIncrement(&_num);
			return nullptr;
		}

		do
		{
			oldtop = _top;
			tempadr = (unsigned long long)oldtop;
			tempadr &= ADRMASK;
			realadr = (Bucket*)tempadr;
			newtop = realadr->_next;
			retptr = realadr->_data;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);
		InterlockedDecrement(&_num);

		_bucketpool.Free(realadr);
		return retptr;
	}

	unsigned long long GetSize()
	{
		return _num;
	}

	int GetCommoncookie()
	{
		return _commoncookie;
	}

private:
	Bucket* _top;							//head인덱스
	unsigned long _num;								//보관 중인 Bucket개수
	short _key;								//tag
	
	//Bucket구조체 관리용 메모리풀
	MemoryPool<Bucket> _bucketpool;
	int _commoncookie;

};





#endif
