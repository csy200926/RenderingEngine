#include "Actor.h"
#pragma once
class Projectile : public MyEngine::Actor
{

	float gravity;

public:
	Projectile(const char * textureName,const float width,const float height);
	~Projectile(void);
};

