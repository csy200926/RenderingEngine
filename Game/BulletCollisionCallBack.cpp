#include "BulletCollisionCallBack.h"
#include "Engine/Actor.h"
#include "Engine/Collision.h"
#include "Engine/MessageManager.h"
void BulletCollisionCallBack::HandleCollision(MyEngine::Actor &i_me, MyEngine::Actor &i_other, float,
											  const MyEngine::Vector3 &)
{
	using namespace MyEngine;
	if (i_other.m_CollisionID == Collision::GetCollisionID("Enemy"))
	{
		MessageManager::SharedMessageManager()->SendAMessage("Hit", i_other, &i_me);
		i_me.getsKilled();
	}
	else if (i_other.m_CollisionID == Collision::GetCollisionID("Wall"))
		i_me.getsKilled();
}
