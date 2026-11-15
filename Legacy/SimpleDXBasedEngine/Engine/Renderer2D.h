#pragma once
#include "Scene.h"
#include <Cheesy.h>
#include "IIsA.h"
namespace MyEngine{

	class Renderer2D: public MyEngine::IIsA
	{

	private:

		MyEngine::Scene *m_pScene;
		MyEngine::TemplateArray<MyEngine::MySharedPointer<MyEngine::Actor>> Actors;
	public:

		Renderer2D(void);
		~Renderer2D(void);

		void draw(  );
		void clear();
		inline void addActor(MyEngine::MySharedPointer<MyEngine::Actor> &);
	
	
		void setRenderQueue(MyEngine::Scene *i_pScene  );
	};
	void Renderer2D::addActor(MyEngine::MySharedPointer<MyEngine::Actor> &i_Actor)
	{
		Actors.push(i_Actor);
	}
}