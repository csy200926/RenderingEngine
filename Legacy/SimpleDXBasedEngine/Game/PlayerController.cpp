#include "PlayerController.h"
#include "Extension.h"
#include "Actor.h"
#include "Vector3.h"
#include "Director.h"
#include "BulletCollisionCallBack.h"
#include "GameWorld.h"
PlayerController::PlayerController(void): IIsA("PlayerController")
{
	bulletCollisionCallBack = new BulletCollisionCallBack();
	shootAngle = 0.0f;
	counter_1 = 0.0f;
	counter_2 = 0.0f;
}


PlayerController::~PlayerController(void)
{
	delete bulletCollisionCallBack;
}

void PlayerController::UpdateActor(MyEngine::Actor & actor , bool & isDone ,double delta)
{

	using namespace MyEngine;

	Director *pd = Director::sharedDirector();

	counter_1 += static_cast<float>( delta );
	counter_2 += static_cast<float>( delta );
	
	Vector3 rotation(0.0f,0.0f,0.0f);

	
	if (pd->isKeyDown(87))// Face up
	{
		shootAngle += 1;
		shootAngle = shootAngle > 45 ? 45 : shootAngle;
	}else if (pd->isKeyDown(83))// Face down
	{
		shootAngle -= 1;
		shootAngle = shootAngle < 0 ? 0 : shootAngle;
	}else if (pd->isKeyPressed(74))// Fire
	{	

		if(counter_1 >= 0.5f)
		{
			counter_1 = 0.0f;
			Vector3 position(-265,-150,0);
			MySharedPointer<Actor> &projectile = GameWorld::getInstance()->shootProjectileAt(position);

			Vector3 velocity(300,50 + shootAngle * 10,0);
			projectile->setVelocity(velocity);

			Vector3 acceleration(0,-7,0);
			projectile->setAcceleration(acceleration);

			projectile->m_CollisionID = Collision::GetCollisionID("Ball");
			projectile->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Enemy");
			projectile->m_collisionCallBack = bulletCollisionCallBack;
		}
	}else if (pd->isKeyPressed(75))// Fire
	{	

		if(counter_2 >= 3.0f)
		{
			counter_2 = 0.0f;
			
			MySharedPointer<Actor> &projectile = GameWorld::getInstance()->useSuperWeapon();

			Vector3 velocity(400,0,0);
			projectile->setVelocity(velocity);

			projectile->m_CollisionID = Collision::GetCollisionID("Ball");
			projectile->m_CollisionLogicalResponseIDMask |= Collision::GetCollisionMask("Enemy");
			projectile->m_collisionCallBack = bulletCollisionCallBack;
		}
	}
	rotation.setZ(shootAngle);
	actor.setRotation(rotation);

}

