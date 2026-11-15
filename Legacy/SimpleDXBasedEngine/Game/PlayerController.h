#pragma once
#include "IActorController.h"
#include "IIsA.h"

class BulletCollisionCallBack;
class PlayerController :
	public MyEngine::IActorController , public MyEngine::IIsA
{
private:
	BulletCollisionCallBack *bulletCollisionCallBack;
	float shootAngle;

	float counter_1;
	float counter_2;

public:
	PlayerController(void);
	~PlayerController(void);
	void UpdateActor(MyEngine::Actor & actor , bool & isDone,double delta);
};

