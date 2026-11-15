#include "BitArray.h"
#include "stdlib.h"
#include "stdio.h"
#include "Extension.h"
#include "AllocationOverride.h"






// Private Constructor
BitArray::BitArray( char * i_pBitMemory,const unsigned int i_itemCount ):
	m_pBitMemory( i_pBitMemory ),
	m_BitCount( i_itemCount )
{
	m_totalChunk = i_itemCount / (sizeof(char) * 8) + 1;
	for ( unsigned int i = 0; i < m_totalChunk; i++ )
		m_pBitMemory[i] = 0;

}

BitArray::~BitArray(void)
{
	free(m_pBitMemory);
}

// Static Create Function
BitArray *BitArray::create(const unsigned int i_itemCount )
{
	// Get a chunk of memory consists of Bit.  The smallest unit malloc can allocate is a Byte.
	//(i_itemCount + 7) is to make sure numbers like 9 can be allocated for 2 Bytes
	unsigned int totalChunk = i_itemCount / (sizeof(char) * 8) + 1;
	void *pBitMemory = malloc( totalChunk * sizeof(char) ); 

	if ( pBitMemory == NULL )
		return NULL;
	else
		return CNEW BitArray((char *)pBitMemory , i_itemCount );

}

bool BitArray::setBit(const unsigned int i_index )
{
	if ( i_index > m_BitCount )
	{
		return false;
	}
	unsigned int chunkCount = i_index / (sizeof(char) * 8);
	unsigned int BitCountInChunk = i_index % (sizeof(char) * 8);
	unsigned char bitMask = 0x1;// BYTE

	bitMask = bitMask<<BitCountInChunk;

	m_pBitMemory[chunkCount] = m_pBitMemory[chunkCount] | bitMask;

	return true;
}

bool BitArray::clearBit(const unsigned int i_index )
{
	unsigned int chunkCount = i_index / (sizeof(char) * 8);
	unsigned int BitCountInChunk = i_index % (sizeof(char) * 8);
	unsigned char bitMask = 0x1;// BYTE

	bitMask = bitMask<<BitCountInChunk;
	bitMask = ~ bitMask;

	m_pBitMemory[chunkCount] = m_pBitMemory[chunkCount] & bitMask;

	return false;
}

unsigned int BitArray::getFirstFreeBit( void )  // start with 0???
{
	unsigned int i_index = 0;

	// Which chunk the first free bit would be in
	while ( i_index < m_totalChunk )
	{
		if ( m_pBitMemory[i_index] !=  0xffffffff )
			break;
		i_index++;
	}
	// If it's full return error
	if (i_index >= m_totalChunk)
		return Error_Full;

	unsigned int counter = 0;
	unsigned char bitMask = 0x1;// BYTE

	while ( m_pBitMemory[i_index] & bitMask ) // if (int)result ==0   then we got the free bit
	{
		bitMask = bitMask << 1;
		counter++;
	}
	if (counter >= 8)
		return Error_Unknow;
	unsigned int result = i_index * (sizeof(char) * 8) + counter;

	// If it's full return error
	if (result >= m_BitCount)
		return Error_Full;
	
	return result;
}

unsigned int BitArray::getFirstSetBit( void )
{
	unsigned int i_index = 0;

	// Which chunk the first set bit would be in
	while ( i_index < m_totalChunk )
	{
		if ( m_pBitMemory[i_index] !=  0x00000000 )
			break;
		i_index++;
	}

	// If it's empty return error
	if (i_index >= m_totalChunk)
		return Error_Empty;

	unsigned int counter = 0;
	unsigned char bitMask = 0x1;// BYTE

	while ( !(m_pBitMemory[i_index] & bitMask) ) // if (int)result ==0   then we got the set bit
	{
		bitMask = bitMask << 1;
		counter++;

	}

	if (counter >= 8)
		return Error_Unknow;

	unsigned int result = i_index * (sizeof(char) * 8) + counter;

	// If it's empty return error
	if (result >= m_BitCount)
		return Error_Empty;

	return result;
}

unsigned int BitArray::getAndClearFirstSetBit( void )
{
	unsigned int result = clearBit(getFirstSetBit());
	return result;
}


unsigned int BitArray::getAndSetFirstFreeBit( void )
{
	unsigned int result = setBit(getFirstFreeBit());
	return result;
}

char BitArray::operator[]( size_t i_index) const
{
	if ( i_index > m_BitCount )
	{
		return false;
	}
	unsigned int chunkCount = i_index / (sizeof(char) * 8);/*******************/
	unsigned int BitCountInChunk = i_index % (sizeof(char) * 8);

	return m_pBitMemory[chunkCount];
}

void BitArray::printOut()
{
	for (unsigned int i = 0; i < (m_BitCount + 7) / 8 ; i++)
	{
		int number = m_pBitMemory[i];
		char string[8];
		_itoa_s(number, string, 2);
		DEBUG_PRINT("Chunk_%d :integer = %8d string = %8s\n", i, number, string);
	}
}