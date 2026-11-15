#pragma once

namespace MyEngine{

template<typename T>
class Singleton
{
public:
	static T* getInstance()
	{
		return m_instance;
	};
	
	static T* CreateInstance()
	{
		if(!m_instance)
			m_instance = new T();

		return m_instance;
	};
	static void release()
	{
		SAFE_DELETE(m_instance);

	}
		
private:

	static T* m_instance;

};
	
template<typename T>
T* Singleton<T>::m_instance = NULL;


}