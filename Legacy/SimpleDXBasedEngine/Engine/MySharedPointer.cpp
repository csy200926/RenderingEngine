#include "MySharedPointer.h"
#include <assert.h>

namespace MyEngine{

	template<class T>
	MySharedPointer<T>::MySharedPointer()
	{
		m_pPtr = NULL;
		m_pReferenceCount = NULL;
	
	}


	template<class T>
	MySharedPointer<T>::MySharedPointer(T *i_pPtr):
		m_pPtr(i_pPtr),
		m_pReferenceCount(new(unsigned int))
	{
		*m_pReferenceCount = 1;

	}

	template<class T>
	MySharedPointer<T>::~MySharedPointer(void)
	{		

		if (m_pPtr == NULL)
			return;

		assert( m_pReferenceCount != NULL );

		(*m_pReferenceCount) --;

		if ((*m_pReferenceCount) == 0)
		{
			if (m_pPtr)
				delete m_pPtr;

			delete m_pReferenceCount;

			m_pPtr = NULL;
			m_pReferenceCount = NULL;
		}
	}

	template<class T>
	MySharedPointer<T>::MySharedPointer(const MySharedPointer<T> &i_other):
		m_pPtr( i_other.m_pPtr ),
		m_pReferenceCount( i_other.m_pReferenceCount )
	{

		assert( m_pReferenceCount != NULL );

		(*m_pReferenceCount) ++;

	}

	template<class T>
	T *MySharedPointer<T>::operator->(void)
	{
		assert(m_pPtr != NULL);
		return m_pPtr;

	}


	template<class T>
	T& MySharedPointer<T>::operator*(void)
	{
		assert(m_pPtr != NULL);
		return *m_pPtr;

	}

	template<class T>
	void MySharedPointer<T>::operator=(const MySharedPointer<T> & i_other)
	{
		if ( this != &i_other )
		{
			if (m_pReferenceCount &&  --(*m_pReferenceCount) == 0)
			{

				if (m_pPtr)
					delete m_pPtr;
				delete m_pReferenceCount;

				m_pPtr = NULL;
				m_pReferenceCount = NULL;
			}

			m_pPtr = i_other.m_pPtr;
			m_pReferenceCount = i_other.m_pReferenceCount;

			(*m_pReferenceCount) ++;
		}

	}

	template<class T>
	bool MySharedPointer<T>::operator==(const MySharedPointer<T> & a_other)const
	{
		if (m_pPtr == a_other.m_pPtr)
			return true;

		return false;
		
	
	}

	template<class T>
	void MySharedPointer<T>::release()
	{
		assert( m_pReferenceCount != NULL );

		(*m_pReferenceCount) --;

		if ((*m_pReferenceCount) == 0)
		{
			if (m_pPtr)
				delete m_pPtr;

			delete m_pReferenceCount;

			m_pPtr = NULL;
			m_pReferenceCount = NULL;

		}

	}

	template<class T>
	unsigned int MySharedPointer<T>::getCount()
	{
		return *m_pReferenceCount;
	
	}
	template<class T>
	void MySharedPointer<T>::retain()
	{
		(*m_pReferenceCount) ++;
	
	}
}