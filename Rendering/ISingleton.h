#pragma once
template <class T> class Singleton
{
  public:
	static T *CreateInstance()
	{
		if (!m_instance)
			m_instance = new T();
		return m_instance;
	}
	static T *GetInstance() { return CreateInstance(); }
	static void DestroyInstance()
	{
		delete m_instance;
		m_instance = nullptr;
	}
	Singleton(const Singleton &) = delete;
	Singleton &operator=(const Singleton &) = delete;

  protected:
	Singleton() = default;
	~Singleton() = default;
	inline static T *m_instance = nullptr;
};
