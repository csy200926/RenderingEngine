#pragma once
#include <utility>
#include <cassert>
namespace MyEngine
{
	// Original reference-counted pointer, with null copy/release and move ownership repaired.
	template <class T> class MySharedPointer
	{
	  public:
		MySharedPointer() = default;
		MySharedPointer(T *i_pPtr) : m_pPtr(i_pPtr), m_pReferenceCount(i_pPtr ? new unsigned(1) : nullptr) {}
		MySharedPointer(const MySharedPointer &i_other)
			: m_pPtr(i_other.m_pPtr), m_pReferenceCount(i_other.m_pReferenceCount)
		{
			retain();
		}
		MySharedPointer(MySharedPointer &&i_other) noexcept { swap(i_other); }
		~MySharedPointer() { release(); }
		MySharedPointer &operator=(MySharedPointer i_other)
		{
			swap(i_other);
			return *this;
		}
		T *operator->() const
		{
			assert(m_pPtr);
			return m_pPtr;
		}
		T &operator*() const
		{
			assert(m_pPtr);
			return *m_pPtr;
		}
		T *get() const { return m_pPtr; }
		explicit operator bool() const { return m_pPtr != nullptr; }
		bool operator==(const MySharedPointer &i_other) const { return m_pPtr == i_other.m_pPtr; }
		unsigned getCount() const { return m_pReferenceCount ? *m_pReferenceCount : 0; }
		void release()
		{
			if (m_pReferenceCount && --*m_pReferenceCount == 0)
			{
				delete m_pPtr;
				delete m_pReferenceCount;
			}
			m_pPtr = nullptr;
			m_pReferenceCount = nullptr;
		}
		void swap(MySharedPointer &i_other) noexcept
		{
			std::swap(m_pPtr, i_other.m_pPtr);
			std::swap(m_pReferenceCount, i_other.m_pReferenceCount);
		}

	  private:
		void retain()
		{
			if (m_pReferenceCount)
				++*m_pReferenceCount;
		}
		T *m_pPtr = nullptr;
		unsigned *m_pReferenceCount = nullptr;
	};
} // namespace MyEngine
