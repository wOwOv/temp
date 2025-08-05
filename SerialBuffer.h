#ifndef __SERIALBUFFER__
#define __SERIALBUFFER__

#include "TlsMemoryPool.h"



class SBuffer
{
public:

	enum en_CBuffer
	{
		BUFFER_DEFAULT = 1024		// ��Ŷ�� �⺻ ���� ������.
	};

	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Return:
	//////////////////////////////////////////////////////////////////////////
	SBuffer();
	SBuffer(int iBufferSize);

	virtual	~SBuffer();


	//////////////////////////////////////////////////////////////////////////
	// ��Ŷ û��.
	//
	// Parameters: ����.
	// Return: ����.
	//////////////////////////////////////////////////////////////////////////
	void	Clear(void);


	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)��Ŷ ���� ������ ���.
	//////////////////////////////////////////////////////////////////////////
	int	GetBufferSize(void);
	//////////////////////////////////////////////////////////////////////////
	// ���� ������� ������ ���.
	//
	// Parameters: ����.
	// Return: (int)������� ����Ÿ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetDataSize(void);



	//////////////////////////////////////////////////////////////////////////
	// ���� ������ ���.
	//
	// Parameters: ����.
	// Return: (char *)���� ������.
	//////////////////////////////////////////////////////////////////////////
	char* GetBufferPtr(void);
	char* GetReadPtr(void);
	char* GetWritePtr();

	//////////////////////////////////////////////////////////////////////////
	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);


	//������ �����ε�
	SBuffer& operator=(SBuffer& srcbuffer);

	SBuffer& operator << (unsigned char value);
	SBuffer& operator << (signed char value);
	SBuffer& operator << (char value);
	//CBuffer& operator<<(BYTE value);

	SBuffer& operator << (short value);
	SBuffer& operator << (unsigned short value);
	//CBuffer& operator << (WORD value);

	SBuffer& operator << (unsigned int value);
	SBuffer& operator << (int value);
	SBuffer& operator << (long value);
	SBuffer& operator << (float value);
	SBuffer& operator << (DWORD value);

	SBuffer& operator << (__int64 iValue);
	SBuffer& operator << (double dValue);


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& operator>>(unsigned char &value);
	SBuffer& operator>>(signed char& value);
	SBuffer& operator>>(char &value);
	//CBuffer& operator>>(BYTE& value);

	SBuffer& operator>>(short &value);
	SBuffer& operator>>(unsigned short &value);
	//CBuffer& operator>>(WORD& value);

	SBuffer& operator >> (unsigned int value);
	SBuffer& operator>>(int &value);
	SBuffer& operator>>(long &value);
	SBuffer& operator>>(float &value);
	SBuffer& operator>>(DWORD& value);

	SBuffer& operator>>(__int64 &value);
	SBuffer& operator>>(double &value);





	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		GetData(char* dest, int size);

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int		PutData(char* src, int srcsize);

	//����ī��Ʈ n ����
	int AddRefcnt(int n);

	//����ī��Ʈ n ����
	int SubRefcnt(int n);

	//����ī��Ʈ 1 ����
	void DecRefcnt();

private:
	void ClearAtLServer(void);
	void ClearAtNServer(void);

protected:
	int	BufferSize;//���� ��ü ������
	char* buffer;
	int read;
	int write;
	
	int	DataSize;//������� ������

	int refcnt;

	char eFlag;
	SRWLOCK eKey;

private:
	static TlsMemoryPool<SBuffer> BufPool;

	friend class CPacket;
	friend class LanServer;
	friend class NetServer;
	friend class CoreServer;


};








#endif