#pragma once
#include "ICollisionCallBack.h"
class BulletCollisionCallBack :public MyEngine::ICollisionCallBack
{
public:
	BulletCollisionCallBack(void);
	~BulletCollisionCallBack(void);

	void HandleCollision( MyEngine::Actor & i_Me, MyEngine::Actor & i_CollidedWith, const float i_CollisionTime, const MyEngine::Vector3 & i_CollisionVector );
};

