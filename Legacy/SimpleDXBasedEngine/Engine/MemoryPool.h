#pragma once
class BitArray;
class MemoryPool
{
private:
	char *m_pMemoryPool;//1 Byte
	BitArray *m_pIsInUse;
	unsigned int m_itemSize;
	unsigned int m_itemCount;

	MemoryPool(char * i_pPoolMemory, BitArray *i_pInUseFlags,const unsigned int i_itemSize ,const unsigned int i_itemCount);
public:

	~MemoryPool(void);

	static MemoryPool *create(const unsigned int i_itemSize ,const unsigned int i_itemCount);

	void *alloc();
	void free( void *i_pPtr );
};

