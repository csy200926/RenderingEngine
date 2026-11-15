#pragma once

namespace MyEngine{

	template <class T>
	class MySharedPointer
	{

		T *m_pPtr;
		unsigned int * m_pReferenceCount;



	public:
		MySharedPointer();
		MySharedPointer(T *i_pPtr);
		~MySharedPointer(void);

		MySharedPointer(const MySharedPointer<T> &i_other);

		T *operator->(void);
		T &operator*(void);
		void operator=(const MySharedPointer<T> & i_other);
		bool operator==(const MySharedPointer<T> & a_other)const;

		void release();

		unsigned int getCount();

		void retain();

	};
	
}
