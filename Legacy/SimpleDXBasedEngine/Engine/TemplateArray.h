#ifndef __TMP_ARRAY__
#define __TMP_ARRAY__

namespace MyEngine{
	template <class T>
	class TemplateArray
	{

	public:
		int len;

		void push(const T &d);
		void remove(const T &d);
		void clear();

		TemplateArray(void);
		~TemplateArray(void);
		T& TemplateArray<T>::operator [](const int index);
	private:
		
		T *m_data;
	};
}


#endif
