#pragma once
namespace MyEngine
{
	template <class T,const int size>
	class RingBuffer
	{
	public:
		RingBuffer(void);
		virtual ~RingBuffer(void);

		unsigned int getSize( void ) const;
		unsigned int getCount(void) const;

		void push( const T & i_item );
		void clear();

		T operator[] ( const unsigned int i_index ) const;


	private:
		T *m_data;

		unsigned int m_count;
	};
}
