#pragma once
#include "Engine/IActorController.h"
#include "BulletCollisionCallBack.h"
class GameWorld;
class PlayerController : public MyEngine::IActorController
{
  public:
	PlayerController(GameWorld &i_world, bool i_autoplay);
	void UpdateActor(MyEngine::Actor &i_actor, bool &o_done, double i_delta) override;
	BulletCollisionCallBack m_bulletCollision;

  private:
	GameWorld &m_world;
	bool m_autoplay;
	float shootAngle = 0, counter_1 = 0, counter_2 = 0;
};
