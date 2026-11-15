#pragma once
#include "Imessagecallback.h"
class MessageTest :
	public MyEngine::IMessageCallBack
{
public:
	MessageTest(void);
	~MessageTest(void);

	void handleMessage(MyEngine::Actor &i_Actor,MyEngine::IIsA*i_pMessageData);
};

