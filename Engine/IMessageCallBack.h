#pragma once
#include "IIsA.h"
namespace MyEngine
{
	class Actor;
	class IMessageCallBack
	{
	  public:
		virtual ~IMessageCallBack() = default;
		virtual void handleMessage(Actor &i_actor, IIsA *i_pMessageData) = 0;
	};
} // namespace MyEngine
