#pragma once
#include "Scene.h"
#include <Cheesy.h>
#include <map>
#include <Mesh.h>
#include "IIsA.h"
namespace MyEngine{

	class Renderer3D: public MyEngine::IIsA
	{

	private:
		typedef std::map<MyEngine::HashedString,MyEngine::MySharedPointer<Cheesy::Mesh>> MeshMap;   
		MeshMap meshMap;

		typedef std::map<MyEngine::HashedString,MyEngine::MySharedPointer<Cheesy::Material>> MaterialMap;   
		MaterialMap materialMap;

		MyEngine::Scene *m_pScene;
		MyEngine::TemplateArray<MyEngine::MySharedPointer<MyEngine::Actor>> Actors;
	public:

		Renderer3D(void);
		~Renderer3D(void);

		void draw(  );
		void clear();
		inline void addActor(MyEngine::MySharedPointer<MyEngine::Actor> &);
	
		void setRenderQueue(MyEngine::Scene *i_pScene  );
	};
	void Renderer3D::addActor(MyEngine::MySharedPointer<MyEngine::Actor> &i_Actor)
	{
		Actors.push(i_Actor);
	}
}