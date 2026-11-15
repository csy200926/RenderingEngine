#pragma once
#include "HashedString.h"
namespace MyEngine
{
	class IIsA
	{
	  public:
		explicit IIsA(const char *i_type) : m_TypeID(i_type) {}
		virtual ~IIsA() = default;
		bool IsA(const HashedString &i_type) const { return m_TypeID == i_type; }

	  private:
		HashedString m_TypeID;
	};
} // namespace MyEngine
