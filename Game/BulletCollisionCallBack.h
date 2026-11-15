#pragma once
#include "Engine/ICollisionCallBack.h"
class BulletCollisionCallBack : public MyEngine::ICollisionCallBack
{
  public:
	void HandleCollision(MyEngine::Actor &i_me, MyEngine::Actor &i_other, float i_time,
						 const MyEngine::Vector3 &i_normal) override;
};
