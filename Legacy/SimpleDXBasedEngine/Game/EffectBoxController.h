#pragma once
#include "IActorController.h"

class EffectBoxController :
	public MyEngine::IActorController
{


public:
	EffectBoxController(void);
	~EffectBoxController(void);

	void UpdateActor(MyEngine::Actor & actor , bool & isDone,double delta);
};

