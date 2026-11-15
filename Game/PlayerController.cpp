#include "PlayerController.h"
#include "GameWorld.h"
#include "Engine/Director.h"
#include <algorithm>
PlayerController::PlayerController(GameWorld &i_world, bool i_autoplay) : m_world(i_world), m_autoplay(i_autoplay)
{
}
void PlayerController::UpdateActor(MyEngine::Actor &i_actor, bool &, double i_delta)
{
	using namespace MyEngine;
	auto *pDirector = Director::sharedDirector();
	float delta = static_cast<float>(i_delta);
	counter_1 += delta;
	counter_2 += delta;
	if (pDirector->isKeyDown('W'))
		shootAngle += 40 * delta;
	if (pDirector->isKeyDown('S'))
		shootAngle -= 40 * delta;
	shootAngle = std::clamp(shootAngle, 0.f, 45.f);
	if ((pDirector->isKeyDown('J') || pDirector->isKeyDown(VK_SPACE) || m_autoplay) && counter_1 >= .5f)
	{
		counter_1 = 0;
		auto projectile = m_world.shootProjectileAt(Vector3(-265, -160, 0));
		projectile->setVelocity(Vector3(380, 40 + shootAngle * 6, 0));
		projectile->setAcceleration(Vector3(0, -70, 0));
	}
	if (pDirector->isKeyPressed('K') && counter_2 >= 3)
	{
		counter_2 = 0;
		m_world.useSuperWeapon();
	}
	i_actor.setRotation(Vector3(0, 0, shootAngle));
}
