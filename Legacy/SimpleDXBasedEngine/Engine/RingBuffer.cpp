#include "RingBuffer.h"
#include "AllocationOverride.h"
#include <assert.h>
namespace MyEngine
{
	template <class T,const int size>
	RingBuffer<T,size>::RingBuffer(void)
	{
		m_count = 0;
		m_data = new T[size];
	}

	template <class T,const int size>
	RingBuffer<T,size>::~RingBuffer(void)
	{
		delete []m_data;
	}

	template <class T,const int size>
	unsigned int RingBuffer<T,size>::getSize( void ) const
	{
		return size;
	}

	template <class T,const int size>
	unsigned int RingBuffer<T,size>::getCount(void) const
	{
		return m_count;
	}


	template <class T,const int size>
	void RingBuffer<T,size>::push( const T & i_item )
	{
		m_data[m_count] = i_item;
		m_count ++;
		if (m_count >= size)
			m_count = 0;
	}


	template <class T,const int size>
	T RingBuffer<T,size>::operator[] (const unsigned int i_index)const
	{
		assert( i_index < size );
		
		int return_count = m_count - i_index - 1;
		if (return_count < 0 )
		{
			return_count = size + return_count;
		}

		return m_data[return_count];
	}

	template <class T,const int size>
	void RingBuffer<T,size>::clear()
	{
		delete []m_data;
		m_data = new T[size];
		m_count = 0;
	}
}