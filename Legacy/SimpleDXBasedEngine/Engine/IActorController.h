#pragma once
#include "IIsA.h"
namespace MyEngine{

	class Actor;
	class IActorController//: public IIsA
	{
		Actor *_pActor;
	public:
		virtual ~IActorController(){};
		virtual void UpdateActor(Actor & actor , bool & isDone , double delta) = 0;
	};

}