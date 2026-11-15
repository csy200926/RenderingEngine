#pragma once

#include "TemplateArray.h"
#include "HashedString.h"
#include <stdio.h>
#include <map>
#include <vector>
#include "IMessageCallBack.h"
#include "IIsA.h"

namespace MyEngine{

class MessageManager
{
	typedef std::map<MyEngine::HashedString, std::vector< IMessageCallBack * >> STRING2VECTOR;    
private:
	//MyEngine::TemplateArray<IMessageCallBack*> MessageCallBacks;
	std::map<MyEngine::HashedString, std::vector< IMessageCallBack * > >MessageCallBacks;
public:
	MessageManager(void);
	~MessageManager(void);
	
	static  MessageManager *SharedMessageManager();

	bool RegisterMessageHandler( const MyEngine::HashedString & i_Message, IMessageCallBack * i_MessageHandler );
	bool UnregisterMessageHandler( const MyEngine::HashedString & i_Message, IMessageCallBack * i_MessageHandler );
	
	bool SendAMessage( const MyEngine::HashedString & i_Message,Actor &i_Actor,IIsA *i_pMessageData);//, IIsA * i_pMessageData, SharedPtr<GameObject> & i_MessageObject );
};

}