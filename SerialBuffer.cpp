#include "SerialBuffer.h"
#include <windows.h>

TlsMemoryPool<SBuffer> SBuffer::BufPool(10000, false);

SBuffer::SBuffer():read(0),write(0),BufferSize(BUFFER_DEFAULT),DataSize(0),refcnt(0), eFlag(0)
{
	//�⺻ ���� ������ �Ҵ�
	buffer = (char*)malloc(BUFFER_DEFAULT);
	InitializeSRWLock(&eKey);
}
SBuffer::SBuffer(int size) :read(0), write(0),BufferSize(size),DataSize(0),refcnt(0),eFlag(0)
{
	//������ BufferSize��ŭ �Ҵ�
	buffer = (char*)malloc(BufferSize);
	InitializeSRWLock(&eKey);
}

SBuffer::~SBuffer()
{
	free(buffer);
}


	// ���� û��.
void SBuffer::Clear(void)
{
	read = 5;
	write =5;
	DataSize = 0;
	eFlag = false;
}

// coreserver ���� û��
void SBuffer::ClearAtLServer(void)
{
	read = 3;
	write = 3;
	DataSize = 0;
	eFlag = false;
}
void SBuffer::ClearAtNServer(void)
{
	read = 0;
	write = 0;
	DataSize = 0;
	eFlag = false;
}

	// ���� ������ ���
	int	SBuffer::GetBufferSize(void)
	{ 
		return BufferSize;
	}

	// ���� ������� ������ ���.
	int	SBuffer::GetDataSize(void)
	{ 
		return DataSize;
	}
	 


	// ���� ������ ���
	char* SBuffer::GetBufferPtr(void)
	{
		return &buffer[0];
	}

	char* SBuffer::GetReadPtr(void)
	{
		return &buffer[read];
	}

	char* SBuffer::GetWritePtr()
	{
		return &buffer[write];
	}


	// ���� Pos �̵�. (�����̵��� �ȵ�)
	// GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
	//
	// Parameters: (int) �̵� ������.
	// Return: (int) �̵��� ������.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::MoveWritePos(int size)
	{
		write += size;
		DataSize += size;
		return size;
	}
	int	SBuffer::MoveReadPos(int size)
	{
		read += size;
		DataSize -= size;
		return size;
	}






	/* ============================================================================= */
	// ������ �����ε�
	/* ============================================================================= */
	SBuffer& SBuffer::operator = (SBuffer& srcbuffer)
	{
		memcpy(GetBufferPtr(), srcbuffer.buffer, srcbuffer.GetBufferSize());
		read = srcbuffer.read;
		write = srcbuffer.write;
		DataSize = srcbuffer.DataSize;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////
	// �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& SBuffer::operator << (unsigned char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[write];
			*temp = value;
			write+=sizeof(value);
			DataSize+= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator<<(signed char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (char value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator << (BYTE value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/




	SBuffer& SBuffer::operator << (short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (unsigned short value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator << (WORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}*/


	SBuffer& SBuffer::operator << (unsigned int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (int value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (long value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (float value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (DWORD value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator << (__int64 value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator << (double value)
	{
		if (BufferSize - DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[write];
			*temp = value;
			write += sizeof(value);
			DataSize += sizeof(value);
		}
		return *this;
	}


	//////////////////////////////////////////////////////////////////////////
	// ����.	�� ���� Ÿ�Ը��� ��� ����.
	//////////////////////////////////////////////////////////////////////////
	SBuffer& SBuffer::operator >> (unsigned char& value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned char* temp = (unsigned char*)&buffer[read];
			value=*temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator>>(signed char& value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (char &value)
	{
		if (DataSize >= sizeof(value))
		{
			char* temp = (char*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator >> (BYTE& value)
	{
		if (DataSize >= sizeof(value))
		{
			BYTE* temp = (BYTE*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/




	SBuffer& SBuffer::operator >> (short &value)
	{
		if (DataSize >= sizeof(value))
		{
			short* temp = (short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	
	SBuffer& SBuffer::operator >> (unsigned short &value)
	{
		if (DataSize >= sizeof(value))
		{
			unsigned short* temp = (unsigned short*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	/*CBuffer& CBuffer::operator >> (WORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			WORD* temp = (WORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}*/



	SBuffer& SBuffer::operator>>(unsigned int value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator >> (int &value)
	{
		if (DataSize >= sizeof(value))
		{
			int* temp = (int*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (long &value)
	{
		if (DataSize >= sizeof(value))
		{
			long* temp = (long*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (float &value)
	{
		if (DataSize >= sizeof(value))
		{
			float* temp = (float*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (DWORD& value)
	{
		if (DataSize >= sizeof(value))
		{
			DWORD* temp = (DWORD*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}

	SBuffer& SBuffer::operator >> (__int64 &value)
	{
		if (DataSize >= sizeof(value))
		{
			__int64* temp = (__int64*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}
	SBuffer& SBuffer::operator >> (double &value)
	{
		if (DataSize >= sizeof(value))
		{
			double* temp = (double*)&buffer[read];
			value = *temp;
			read += sizeof(value);
			DataSize -= sizeof(value);
		}
		return *this;
	}




	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ���.
	//
	// Parameters: (char *)Dest ������. (int)Size.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::GetData(char* dest, int size)
	{

		//������� ������ 0�� ��
		if (DataSize == 0)
		{
			return 0;
		}

		//������� ������ ���� �����;纸�� Ŭ ��
		if (DataSize >= size)
		{
			memcpy(dest, &buffer[read], size);
			read += size;
			DataSize -= size;
			return size;
		}
		//������� ������ ���� �����;纸�� �۾Ƽ� �׳� �ִ°� �� ���ٶ�..?
		else
		{
			memcpy(dest, &buffer[read], DataSize);
			read += DataSize;
			DataSize = 0;
			return DataSize;
		}


	}

	//////////////////////////////////////////////////////////////////////////
	// ����Ÿ ����.
	//
	// Parameters: (char *)Src ������. (int)SrcSize.
	// Return: (int)������ ������.
	//////////////////////////////////////////////////////////////////////////
	int	SBuffer::PutData(char* src, int srcsize)
	{
	//���� �� �ִ� ������ ������
		if (DataSize == BufferSize)
		{
			return 0;
		}
		//���� �� �ִ� ������ ����� ��
		if (BufferSize - DataSize >= srcsize)
		{
			memcpy(&buffer[write], src, srcsize);
			write += srcsize;
			DataSize += srcsize;
			return srcsize;
		}
		//���� �� �մ� ������ ������ �� �ִ��� �־��ֱ�
		else
		{
			int size = BufferSize - DataSize;
			memcpy(&buffer[write], src, size);
			write += size;
			DataSize += size;
			return size;
		}
	}

	int SBuffer::AddRefcnt(int n)
	{
		int ret=InterlockedAdd((LONG*)&refcnt, n);

		return ret;
	}

	int SBuffer::SubRefcnt(int n)
	{
		int ret=InterlockedAdd((LONG*)&refcnt, -n);
		
		return ret;
	}

	void SBuffer::DecRefcnt()
	{
		if (InterlockedDecrement((LONG*)&refcnt) == 0)
		{
			BufPool.Free(this);
		}
	}












