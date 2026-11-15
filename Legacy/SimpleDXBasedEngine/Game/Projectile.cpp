#include "Projectile.h"


Projectile::Projectile(const char * textureName,const float width,const float height)
	:MyEngine::Actor(textureName,width,height,false)
{
	gravity = 0.1f;

}


Projectile::~Projectile(void)
{
}
