#pragma once
#include "Scene.h"
#include <Cheesy.h>
#include "NamedBitSet.h"
#include "ICollisionCallBack.h"

namespace MyEngine{

	class Collision
	{

	private:

		MyEngine::TemplateArray<MyEngine::ICollisionCallBack> Handlers;

		MyEngine::TemplateArray<MyEngine::MySharedPointer<MyEngine::Actor>> Actors;
	public:
		Collision(void);

		~Collision(void);

		void update(const double delta);

		void clear();
		inline void addActor(MyEngine::MySharedPointer<MyEngine::Actor> &);

		
		static unsigned int GetCollisionID( const HashedString & i_TypeHash );

		static unsigned int GetCollisionMask( const HashedString & i_TypeHash );

		static unsigned int GetCollisionMask( unsigned int i_ID );
	};
	
	void Collision::addActor(MyEngine::MySharedPointer<MyEngine::Actor> &i_Actor)
	{
		Actors.push(i_Actor);
	}

}