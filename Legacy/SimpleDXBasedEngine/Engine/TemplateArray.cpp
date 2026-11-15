#ifndef __TMP_ARRAY_CPP__
#define __TMP_ARRAY_CPP__

#include "TemplateArray.h"
#include "stdio.h"
#include "AllocationOverride.h"

namespace MyEngine{

	template <class T>
	TemplateArray<T>::TemplateArray(void)
	{
		m_data = NULL;
		len = 0;
	}

	template <class T>
	TemplateArray<T>::~TemplateArray(void)
	{
		delete[] m_data;
	}

	template <class T>
	void TemplateArray<T>::push(const T &d)
	{
		// 1. create a new array which is one unit bigger than the old one 
		// 2. copy the old array to the new one
		// 3. put the variable to the last space of new array
		T *pdata = m_data;

		m_data = new T[len + 1];

		if(pdata != NULL)
		{
			for(int i = 0 ; i < len ; i++)
			{
				m_data[i] = pdata[i];
			}
			delete[] pdata;
		}
		m_data[len] = d;
		len++;
	}

	template <class T>
	void TemplateArray<T>::remove(const T &d)
	{
		int i = 0;
		for( ; i < len ; i++)
		{
			if (d == m_data[i])
			{
				break;
			}
		}


		T *pdata = m_data;

		m_data = new T[len - 1];
		len--;// didn't delete the element. Plz remember to delete it.
		if(pdata != NULL)
		{
			for(int j = 0 ; j < len ; j++)
			{
				if (j >= i)
					m_data[j] = pdata[j + 1];
				else
					m_data[j] = pdata[j];
			}
			delete[] pdata;
		}
		
	}
	
	template <class T>
	void TemplateArray<T>::clear()
	{
		delete[] m_data;
		m_data = NULL;

	}


	template <class T>
	T& TemplateArray<T>::operator [](const int index)
	{
		if(index<0||index>(len-1))
		{
			exit(1);
		}
		return m_data[index];
	}
}
#endif
