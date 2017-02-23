#pragma once
#include "Game.h"

class RenderingGame : public Game
{
public:

protected:

	virtual void OnStart();
	virtual void OnDestroy();
	virtual void Update();

private:


	SceneNode *pTestNode;

};