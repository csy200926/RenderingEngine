#include "GameWorld.h"
#include "Director.h"
#include "EffectBox.h"
#include "EffectBoxController.h"
GameWorld::GameWorld(void)
{
	playerInfo.bulletNumber = 10;
	playerInfo.healthPoint = 100;
	effectBoxController = new EffectBoxController();
	counter = 0;
}


GameWorld::~GameWorld(void)
{
	SAFE_DELETE(effectBoxController);

}

MyEngine::MySharedPointer<MyEngine::Actor> GameWorld::shootProjectileAt(MyEngine::Vector3 &position)
{
	using namespace MyEngine;

	Director *pd = Director::sharedDirector();

	Actor *actor = new Actor("image/PlayerBullet.png",20,20,false);
	actor->setPosition(position);


	MySharedPointer<Actor> sActor(actor);
	pd->addActor(sActor);

	

	return sActor;

}
MyEngine::MySharedPointer<MyEngine::Actor> GameWorld::useSuperWeapon(  )
{
	using namespace MyEngine;

	Vector3 position(-225,-130,0);

	Director *pd = Director::sharedDirector();

	Actor *actor = new Actor("image/PlayerBullet.png",100,100,false);
	actor->setPosition(position);
	actor->setMass(1000);

	MySharedPointer<Actor> sActor(actor);
	pd->addActor(sActor);

	return sActor;

}

void GameWorld::update(const double dt)
{
	Scene::update(dt);


	counter += static_cast<float>( dt );
	if(counter >= 0.5f + (rand()/(RAND_MAX+1.0) * 100))
	{
		using namespace MyEngine;

		int type = static_cast<int>( rand()/(RAND_MAX+1.0) * 10);

		const char * textureName = "image/Slim.png";
		int tag = 1;

		Vector3 position(200,-160,0);


		if( type > 5 )
		{
			position.setValue(200,-100,0);
			textureName = "image/ghost.png";
			tag = 2;
		}

		Director *pd = Director::sharedDirector();

		Vector3 velocity(-50,0,0);

		Actor *actor = new Actor(textureName,20,20,false);
		actor->m_CollisionID = Collision::GetCollisionID("Enemy");
		actor->setTag(tag);

		MySharedPointer<Actor> sActor(actor);
		pd->addActor(sActor);
		actor->setPosition(position);
		actor->setVelocity(velocity);
		counter = 0.0f;
	}



}

void GameWorld::expoldeEffect(MyEngine::Vector3 &position , int type)
{
	using namespace MyEngine;

	Director *pd = Director::sharedDirector();

	float x = position.getX()/800 * 22,y = position.getY()/600 * 16;
	Vector3 velocity(x,y,0);

	const char * materialName;

	if(type == 1)
		materialName = "MaterialSlim";
	else
		materialName = "MaterialGhost";


	EffectBox *actor = new EffectBox(materialName,20,20);
	actor->addController(effectBoxController);
	actor->setLifeSpan(1.0f);

	MySharedPointer<Actor> sActor(actor);
	pd->addActor(sActor);
	actor->setPosition(velocity);


}