#include "Renderer2D.h"
#include "Actor.h"

namespace MyEngine{

	Renderer2D::Renderer2D(void): IIsA("Renderer2D")
	{
	}

	void Renderer2D::clear(void)
	{

		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
			//Actors[i].release();
			Actors.remove(Actors[i]);
		}
	}

	Renderer2D::~Renderer2D(void)
	{

		clear();
	}

	void Renderer2D::draw(  )
	{
		using namespace MyEngine;

		struct Cheesy::Point2D	offsetBG( 0.0f, 0.0f );

		// mix map with Actor	
		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
		
			MySharedPointer<Actor> &s = Actors[i];

			if (s->getState() == ActorState_Dead )
			{
				Actors.remove(s);
				continue;
			}
			if(s->isTriggerBox)
				continue;
			offsetBG.x = s->getPosition()->getX();
			offsetBG.y = s->getPosition()->getY();

			s->getSprite()->Draw(offsetBG,s->getRotation()->getZ());
		}

	}

	void Renderer2D::setRenderQueue(MyEngine::Scene *i_pScene  )
	{
		m_pScene = i_pScene;

	}

}