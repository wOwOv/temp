#include "RingBuffer.h"
#include <malloc.h>
#include <windows.h>

//������ ������ - �⺻ �Ҵ�(SIZE��ŭ)
RingBuffer::RingBuffer():_front(0),_rear(0),_size(DEF)
{
	_buffer = (char*)malloc(sizeof(char) * DEF);
}

//������ ������ - ���ϴ� ũ�� �Ҵ�
RingBuffer::RingBuffer(int buffersize) :_front(0), _rear(0), _size(buffersize)
{
	_buffer = (char*)malloc(sizeof(char) * buffersize);
}

//������ �Ҹ���
RingBuffer::~RingBuffer()
{
	free(_buffer);
}

//�Ҵ� �����ؼ� ����, _front, _rear �缼��
void RingBuffer::Resize(int size)
{
	int deqret;
	char* temp = new char[_size];
	int usedsize = GetUsedSize();
	deqret = Dequeue(temp, usedsize);
	if (deqret != usedsize)
	{
		DebugBreak();
	}
	free(_buffer);
	_buffer = new char[size];
	memcpy(_buffer, temp, deqret);
	_front = 0;
	_rear = deqret;
}


//�������� ��ü ������ ���
int RingBuffer::GetBufferSize()
{
	return _size-1;
}
//�������� ����� ũ��
int RingBuffer::GetUsedSize()
{
	int fixedfront = _front;
	int fixedrear = _rear;

	if (fixedfront <= fixedrear)
	{
		return fixedrear - fixedfront;
	}
	else
	{
		return _size - (fixedfront - fixedrear);
	}
}
//�������� �̻�� ũ��
int RingBuffer::GetFreeSize()
{
	return (_size - GetUsedSize() - 1);
}


//�����۴� ����ť�ΰŴϱ� �ּ��� ���� �־��ִ°ɷ�
// �ϴ��� �� ���� �������ڷ�...
//�����ۿ� ��ť
int RingBuffer::Enqueue(const char* data, int size)
{
	int freesize = GetFreeSize();
	if (freesize >= size)
	{
		if (_rear < _front)
		{
			memcpy(&_buffer[_rear], data, size);
			_rear = (_rear + size) % _size;
			return size;
		}
		else
		{
			if ((_size - _rear) >= size)
			{
				memcpy(&_buffer[_rear], data, size);
				_rear = (_rear + size) % _size;
				return size;
			}

			int remainbyte = size;
			int movebyte = _size - _rear;
			memcpy(&_buffer[_rear], data, movebyte);
			data += movebyte;
			remainbyte -= movebyte;
			memcpy(&_buffer[0], data, remainbyte);
			_rear = (_rear + size) % _size;
			return size;
		}
	}
	else
	{
		return 0;
	}
}
//�׳� �ִ��� �ִ°� �ֱ�                         ------O
//�ϴ� �� size��ŭ �� �ָ� return 0;...          ------X
int RingBuffer::Dequeue(char* dest, int size)
{
	if (GetUsedSize() >= size)
	{
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], size);
			_front = (_front + size) % _size;
			return size;
		}
		else
		{
			if ((_size - _front) >= size)
			{
				memcpy(dest, &_buffer[_front], size);
				_front = (_front + size) % _size;
				return size;
			}
			int remainbyte = size;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			_front = (_front + size) % _size;
			return size;
		}
	}
	else
	{
		//return 0;

		int usedsize = GetUsedSize();
		if (usedsize == 0)
		{
			return 0;
		}
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], usedsize);
			_front = (_front + usedsize) % _size;
			return usedsize;
		}
		else
		{
			int remainbyte = usedsize;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			_front = (_front + usedsize) % _size;
			return usedsize;
		}
	}
}

//�ִ��� �ֱ�
//�����ʹ� ������ front�� ���������� ����
int RingBuffer::Peek(char* dest, int size)
{
	if (GetUsedSize() >= size)
	{
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], size);
			return size;
		}
		else
		{
			if ((_size - _front) >= size)
			{
				memcpy(dest, &_buffer[_front], size);
				return size;
			}
			int remainbyte = size;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			return size;
		}
	}
	else
	{
		//return 0;

		int usedsize = GetUsedSize();
		if (usedsize == 0)
		{
			return 0;
		}
		if (_front < _rear)
		{
			memcpy(dest, &_buffer[_front], usedsize);
			return usedsize;
		}
		else
		{
			int remainbyte = usedsize;
			int movebyte = _size - _front;
			memcpy(dest, &_buffer[_front], movebyte);
			dest += movebyte;
			remainbyte -= movebyte;
			memcpy(dest, &_buffer[0], remainbyte);
			return usedsize;
		}
	}
}

//������ ��� ������ ����
void RingBuffer::ClearBuffer()
{
	_front = _rear = 0;
}

int RingBuffer::DirectEnqueueSize()
{
	if (_rear < _front)
	{
		return _front - _rear - 1;
	}
	else
	{
		if (_front == 0)
		{
			return _size - _rear - 1;
		}
		else
		{
		return _size - _rear;
		}
		
	}
	
}
int RingBuffer::DirectDequeueSize()
{
		if (_front < _rear)
		{
			return _rear - _front;
		}
		else
		{
			return _size - _front;
		}
}

//�������� rear ����
int RingBuffer::MoveRear(int size)
{
	if (this->GetFreeSize() >= size)
	{
		_rear = (_rear + size) % _size;
		return size;
	}
	else
	{
		if (_rear <= _front)
		{
			int byte = _front - _rear - 1;
			_rear = _front - 1;
			return byte;
		}
		else
		{
			int byte = (_size - _rear) + _front - 1;
			_rear = _front - 1;
			return byte;
		}
	}
}
//�������� front ����
int RingBuffer::MoveFront(int size)
{
	if (this->GetUsedSize() >= size)
	{
		_front = (_front + size) % _size;
		return size;
	}
	else
	{
		if (_front <= _rear)
		{
			int byte = _rear - _front;
			_front = _rear;
			return byte;
		}
		else
		{
			int byte = (_size - _front) + _rear;
			_front = _rear;
			return byte;
		}
	}
}
//������ front������ ���
char* RingBuffer::GetFrontBufferPtr()
{
	return &_buffer[_front];
}
//������ rear������ ���
char* RingBuffer::GetRearBufferPtr()
{
	return &_buffer[_rear];
}
//������ ù ��� ������ ���
char* RingBuffer::GetStartBufferPtr()
{
	return &_buffer[0];
}
