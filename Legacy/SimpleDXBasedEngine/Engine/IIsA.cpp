#include "IIsA.h"

namespace MyEngine{

	IIsA::IIsA(const HashedString & i_TypeID)
	{
		m_TypeID = i_TypeID;
	}
	IIsA::IIsA(const char * i_TypeString)
	{
		m_TypeID = HashedString(i_TypeString);
	}


	IIsA::~IIsA(void)
	{

	}


}