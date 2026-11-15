#pragma once

#include "Actor.h"
#include "MySharedPointer.h"
#include "IIsA.h"


namespace MyEngine{

class IMessageCallBack
{
public:

	virtual ~IMessageCallBack(void){};

	virtual void handleMessage(Actor &i_Actor,IIsA *i_pMessageData) = 0;


};

}