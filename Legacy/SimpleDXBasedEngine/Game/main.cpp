#include <iostream>

#include "Director.h"
#include "Vector3.h"


#include "PlayerController.h"
#include "EffectBoxController.h"
#include <crtdbg.h>
#include "Extension.h"
#include <windows.h>
#include <stdio.h>

#include "Collision.h"
#include "BulletCollisionCallBack.h"

#include "MessageTest.h"
#include "MathStuff.h"

#include "ScopedTimer.h"
#include "Profiling.h"
#include "WinTiming.h"
#include "GameWorld.h"


using namespace std;
using namespace MyEngine;

#define CAPTURE_PLAYER_MOVEMENT

//struct Cheesy::_MeshData CubeData;

//*******************version 2.06************************
//
//		Vector4  and   Matrix4x4  implemented
//
//		
//		UnitTest tested the functions.
//		
//		
//
//*******************************************************



int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	
#ifdef ENABLE_PROFILING
AllocConsole();
freopen("conout$","w",stdout);
#endif
	
	const int mapWidth = 20;
	const int mapHeight = 20;
	PlayerController *pP = new PlayerController();

	// Load the world from Lua File
	Director *pDirector = CNEW Director("Data/Config.lua");
	
	MessageTest *msgT = new MessageTest();
	MessageManager::SharedMessageManager()->RegisterMessageHandler(HashedString("Hit"),msgT);
	
	GameWorld *pNewScene = GameWorld::CreateInstance();
	{
		Vector3 pos(-300,-150,0);

		MySharedPointer<Actor> &backGround = pNewScene->addActor("image/Logo.png",610,74,false);
		

		MySharedPointer<Actor> &player = pNewScene->addActor("image/player.png",30,30,false);
		player->setPosition(pos);
		MySharedPointer<Actor> &gun = pNewScene->addActor("image/Gun.png",30,30,false);
		pos.setValue(-280,-150,0);
		gun->setMass(10.0f);
		gun->setPosition(pos);
		gun->addController(pP);

		MySharedPointer<Actor> &wallLeft = pNewScene->addTriggerBox(0,300,0,false);
		wallLeft->m_CollisionID = Collision::GetCollisionID("Wall");
		wallLeft->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Ball");
		wallLeft->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Paddle");

		pos.setValue(-300,0,0);
		wallLeft->setPosition(pos);

		MySharedPointer<Actor> &wallRight = pNewScene->addTriggerBox(0,300,0,false);
		wallRight->m_CollisionID = Collision::GetCollisionID("Wall");
		wallRight->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Ball");
		wallRight->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Paddle");

		pos.setValue(300,0,0);
		wallRight->setPosition(pos);

		MySharedPointer<Actor> &wallUp = pNewScene->addTriggerBox(300,0,0,false);
		wallUp->m_CollisionID = Collision::GetCollisionID("Wall");
		wallUp->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Ball");
		wallUp->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Paddle");
		pos.setValue(0,200,0);wallUp->boundingBox.HalfX = 600.0f;

		wallUp->setPosition(pos);


		MySharedPointer<Actor> &wallDown = pNewScene->addTriggerBox(300,0,0,false);
		wallDown->m_CollisionID = Collision::GetCollisionID("Wall");
		wallDown->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Ball");
		wallDown->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Paddle");
		pos.setValue(0,-200,0);wallDown->boundingBox.HalfX = 600.0f;

		wallDown->setPosition(pos);

	}

	// Run main loop
	pDirector->pushScene(pNewScene);

	pDirector->run();

	delete pP;
	GameWorld::release();
	delete pDirector;

	
	Actor::releaseMemoryPool();
	
	

Cheesy::Shutdown();
#ifdef _DEBUG 
_CrtDumpMemoryLeaks();
#endif


#ifdef ENABLE_PROFILING
PROFILE_PRINT_RESULTS
system("Pause");
#endif
	
	return 0;
}


