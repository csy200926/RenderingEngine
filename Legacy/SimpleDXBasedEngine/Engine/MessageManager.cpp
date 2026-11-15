#include "MessageManager.h"

namespace MyEngine{
static MessageManager *pSharedMessageManager;


MessageManager *MessageManager::SharedMessageManager()

{

	return pSharedMessageManager;
}

MessageManager::MessageManager(void)
{

	pSharedMessageManager = this;

}


MessageManager::~MessageManager(void)
{
	using namespace std;
	using namespace MyEngine;
	
	STRING2VECTOR::iterator iter =  MessageCallBacks.begin(); 

	while (iter != MessageCallBacks.end())
	{
		// Check if i_messageHandler already exists.
		vector<IMessageCallBack *>::iterator it = iter->second.begin();
		while (it != iter->second.end())
		{
			delete *it;
			it ++;
		}
		iter++;
	}
}

bool MessageManager::RegisterMessageHandler( const MyEngine::HashedString & i_Message, IMessageCallBack * i_MessageHandler )
{
	using namespace std;
	using namespace MyEngine;


	STRING2VECTOR::iterator iter = MessageCallBacks.find( i_Message );  

	if (iter != MessageCallBacks.end())
	{
		// Check if i_messageHandler already exists.
		vector<IMessageCallBack *>::iterator it = iter->second.begin();
		while (it != iter->second.end())
		{
			if(*it == i_MessageHandler)
				return true;
			it ++;
		}
		
		// insert a new handler for i_message
		iter->second.insert(iter->second.end(),i_MessageHandler);
	}else
	{
		
		// insert the new message and handler directly
		vector<IMessageCallBack *> newIMCV;
		newIMCV.insert(newIMCV.end(),i_MessageHandler);
		MessageCallBacks.insert( STRING2VECTOR::value_type ( i_Message, newIMCV ));
	}

	return false;
}
bool MessageManager::UnregisterMessageHandler( const MyEngine::HashedString & i_Message, IMessageCallBack * i_MessageHandler )
{
	return false;
}
	
bool MessageManager::SendAMessage( const MyEngine::HashedString & i_Message,Actor &i_Actor,IIsA *i_pMessageData)
{
	using namespace std;
	using namespace MyEngine;
	
	STRING2VECTOR::iterator iter = MessageCallBacks.find( i_Message );  

	if (iter != MessageCallBacks.end())
	{
		// Check if i_messageHandler already exists.
		vector<IMessageCallBack *>::iterator it = iter->second.begin();
		while (it != iter->second.end())
		{
			(*it)->handleMessage(i_Actor,i_pMessageData);
				return true;
			it ++;
		}
		
	}
	return false;

}
}