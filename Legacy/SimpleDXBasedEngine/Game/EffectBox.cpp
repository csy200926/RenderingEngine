#include "EffectBox.h"
#include "Vector3.h"

EffectBox::EffectBox(const char * textureName,const float width,const float height)
	:MyEngine::Actor(textureName,width,height,true)
{


	float x = static_cast<float>( (rand()/(RAND_MAX+1.0) * 10.0f));
	float y = static_cast<float>( -(rand()/(RAND_MAX+1.0) * 10.0f));
	float z = static_cast<float>( -(rand()/(RAND_MAX+1.0) * 10));
	MyEngine::Vector3 velocity(x,y,z);

	setVelocity(velocity);
}


EffectBox::~EffectBox(void)
{
}
