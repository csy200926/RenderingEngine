#ifndef _SINGLETON_H__
#define _SINGLETON_H__

#include <assert.h>

template <typename T> class Singleton
{
private:
	/** \brief Explicit private copy constructor. This is a forbidden operation.*/
	Singleton(const Singleton<T> &);

	/** \brief Private operator= . This is a forbidden operation. */
	Singleton& operator=(const Singleton<T> &);

protected:

	static T* ms_Singleton;

public:
	Singleton(void)
	{
		assert(!ms_Singleton);
		ms_Singleton = static_cast< T* >(this);
	}
	~Singleton(void)
	{
		assert(ms_Singleton);  ms_Singleton = 0;
	}
	static T& getSingleton(void)
	{
		assert(ms_Singleton);  return (*ms_Singleton);
	}
	static T* getSingletonPtr(void)
	{
		return ms_Singleton;
	}
};

#endif
