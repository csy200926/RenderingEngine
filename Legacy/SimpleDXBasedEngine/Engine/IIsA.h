#pragma once
#include "HashedString.h"
namespace MyEngine{

class IIsA
{
	HashedString m_TypeID;
public:
	inline IIsA(const HashedString & i_TypeID);
	IIsA(const char  * i_TypeString);
	~IIsA(void);


	inline bool IsA(const HashedString & i_TypeID) const;

};
	bool IIsA::IsA(const HashedString & i_TypeID) const
	{
 
		if (i_TypeID == m_TypeID)
		{
			return true;
		}
		return false;
	}
}

