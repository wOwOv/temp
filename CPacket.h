#ifndef __CPACKET__
#define __CPACKET__


class CPacket
{
	friend class LanServer;
	friend class NetServer;
	friend class CoreServer;
	friend class SBuffer;

public:
	CPacket();

	CPacket(SBuffer* buf);

	CPacket(CPacket& buf);

	CPacket(CPacket* buf);


	~CPacket();

	CPacket& operator=(const CPacket& buf);

	CPacket* MakePacketPtr();
	static CPacket* GetPacketPtr(CPacket* packet);

	void	Clear(void);

	// ���� ������ ���.
	int	GetBufferSize(void);

	// ���� ������� ������ ���.
	int		GetDataSize(void);



	// ���� ������ ���.
	char* GetBufferPtr(void);
	char* GetReadPtr(void);
	char* GetWritePtr(void);

	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	int		MoveWritePos(int iSize);
	int		MoveReadPos(int iSize);


	//������ �����ε�
	CPacket& operator=(CPacket& srcbuffer);

	CPacket& operator << (unsigned char value);
	CPacket& operator << (signed char value);
	CPacket& operator << (char value);
	//CPacket& operator<<(BYTE value);

	CPacket& operator << (short value);
	CPacket& operator << (unsigned short value);
	//CPacket& operator << (WORD value);

	CPacket& operator << (unsigned int value);
	CPacket& operator << (int value);
	CPacket& operator << (long value);
	CPacket& operator << (float value);
	//CPacket& operator << (DWORD value);

	CPacket& operator << (__int64 value);
	CPacket& operator << (double value);


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	CPacket& operator>>(unsigned char& value);
	CPacket& operator>>(signed char& value);
	CPacket& operator>>(char& value);
	//CPacket& operator>>(BYTE& value);

	CPacket& operator>>(short& value);
	CPacket& operator>>(unsigned short& value);
	//CPacket& operator>>(WORD& value);

	CPacket& operator >> (unsigned int value);
	CPacket& operator>>(int& value);
	CPacket& operator>>(long& value);
	CPacket& operator>>(float& value);
	//CPacket& operator>>(DWORD& value);

	CPacket& operator>>(__int64& value);
	CPacket& operator>>(double& value);


	// ����Ÿ ���.
	int		GetData(char* dest, int size);

	// ����Ÿ ����.
	int		PutData(char* src, int srcsize);



private:
	SBuffer* SBuf;



};

#endif
