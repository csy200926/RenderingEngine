#include "MessageTest.h"
#include "Extension.h"
#include "HashedString.h"
#include "GameWorld.h"

MessageTest::MessageTest(void)
{
}


MessageTest::~MessageTest(void)
{
}


void MessageTest::handleMessage(MyEngine::Actor &i_Actor,MyEngine::IIsA*i_pMessageData)
{

	if (i_pMessageData->IsA(MyEngine::HashedString("Actor")))
	{
		DEBUG_PRINT(" Object: %d has been hit!! \n",i_Actor.m_CollisionID);
		// Do something..........
		i_Actor.getsKilled();		
		GameWorld::getInstance()->expoldeEffect(*i_Actor.getPosition(),i_Actor.getTag());
	}


}