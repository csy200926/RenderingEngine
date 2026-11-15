#pragma once
#include "stdint.h"

typedef enum Error
{
	Error_Full = 99999,
	Error_Empty,
	Error_Unknow
};

class BitArray
{
	char *m_pBitMemory; // unsigned int is not necessarily 32 on other platforms
	unsigned int m_BitCount;
	unsigned int m_totalChunk;

	BitArray( char * i_pBitMemory,const unsigned int i_itemCount );
	

public:

	inline void printOut();

	unsigned int getFirstFreeBit( void );
	inline unsigned int getAndSetFirstFreeBit( void );

	unsigned int getFirstSetBit( void );
	inline unsigned int getAndClearFirstSetBit( void );

	bool setBit(const unsigned int i_index );
	bool clearBit(const unsigned int i_index );

	static BitArray * create(const unsigned int i_itemCount );
	char operator[]( size_t ) const; // ReadOnly
	~BitArray(void);
};

