#pragma once
#include "Scene.h"
#include "Singleton.h"
class EffectBoxController;
class GameWorld :
	public MyEngine::Scene,public MyEngine::Singleton<GameWorld>
{
	struct PlayerInfo
	{
		int bulletNumber;
		int healthPoint;


	};
private:
	float counter;
	EffectBoxController *effectBoxController;
	PlayerInfo playerInfo;

public:
	
	GameWorld(void);
	~GameWorld(void);

	virtual void update(const double dt);

	MyEngine::MySharedPointer<MyEngine::Actor> shootProjectileAt( MyEngine::Vector3 &position );
	MyEngine::MySharedPointer<MyEngine::Actor> useSuperWeapon(  );

	inline void addHealthPoint( int amount );
	inline void addBullets( int amount );

	inline int getHealthPoint();
	inline int getBulletNumber();

	void expoldeEffect(MyEngine::Vector3 &position , int type);

};

void GameWorld::addHealthPoint( int amount )
{
	playerInfo.bulletNumber += amount;
}

void GameWorld::addBullets( int amount )
{
	playerInfo.healthPoint += amount;
}

int GameWorld::getHealthPoint()
{
	return playerInfo.bulletNumber;
}
int GameWorld::getBulletNumber()
{
	return playerInfo.healthPoint;
}