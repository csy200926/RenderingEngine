#include "BulletCollisionCallBack.h"
#include "Collision.h"
#include "MessageManager.h"
#include "PlayerController.h"

using namespace MyEngine;
BulletCollisionCallBack::BulletCollisionCallBack(void)
{
}


BulletCollisionCallBack::~BulletCollisionCallBack(void)
{
}

void BulletCollisionCallBack::HandleCollision( MyEngine::Actor & i_Me,  MyEngine::Actor & i_CollidedWith, const float i_CollisionTime, const MyEngine::Vector3 & i_CollisionVector )
{
	
	if (i_CollidedWith.m_CollisionID == Collision::GetCollisionID("Enemy"))
	{

		MessageManager::SharedMessageManager()->SendAMessage(HashedString("Hit"),i_CollidedWith,&i_Me);

	}else if(i_CollidedWith.m_CollisionID == Collision::GetCollisionID("Wall"))
	{

		i_Me.getsKilled();
	}
	

}