#ifndef __BUCKETSTACK__
#define __BUCKETSTACK__

static unsigned long commoncookie = 0x01010100;


//락프리스택
template <class DATA>
class BucketStack
{
private:
	struct Node
	{
		int _underguard;
		DATA _data;
		int _overguard;
		Node* _next;
		Node* _bucketnext;
	}typedef Bucket;

	const unsigned long long ADRMASK = 0x0000ffffffffffff;

public:
	BucketStack() :_top(nullptr), _num(0), _key(0)
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

	void ReturnBucket(void* node)
	{
		Bucket* bucket = (Bucket*)node;
		unsigned long long tagadr = (unsigned long long)bucket;
		unsigned long long tag = InterlockedIncrement16(&_key);
		tagadr |= (tag << 48);		
		Bucket* tagbucket = (Bucket*)tagadr;
		Bucket* oldtop;
		do
		{
			oldtop = _top;
			bucket->_bucketnext = oldtop;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)tagbucket, (__int64)oldtop) != (__int64)oldtop);
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
			newtop = realadr->_bucketnext;
			retptr = realadr;
		} while (InterlockedCompareExchange64((__int64*)&_top, (__int64)newtop, (__int64)oldtop) != (__int64)oldtop);

		return retptr;
	}

	unsigned long long GetSize()
	{
		return _num;
	}

	int GetCommonCookie()
	{
		return _commoncookie;
	}

private:
	Bucket* _top;								//head
	unsigned long _num;						//보관 중인 Bucket개수
	short _key;								//tag
	
	int _commoncookie;

};





#endif
