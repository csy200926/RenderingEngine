#pragma once
#include "Scene.h"
namespace MyEngine{

	void Scene::addActor(MySharedPointer<Actor> & i_Actor)
	{
		Actors.push(i_Actor);
	}
	MySharedPointer<Actor> Scene::addActor(const char * i_name,const float width,const float height,bool is3DObj)
	{
		MySharedPointer<Actor> pA(new Actor(i_name,width,height,is3DObj));
		Actors.push(pA);
		return pA;
	}

	MySharedPointer<Actor> Scene::addTriggerBox(float Bx,float By,float Bz,bool is3DObj)
	{
		MySharedPointer<Actor> pA(new Actor(Bx,By,Bz,is3DObj));
		pA->isTriggerBox = true;
		Actors.push(pA);
		return pA;
	}




}