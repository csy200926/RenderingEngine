#pragma once
#include "TemplateArray.h"
#include "TemplateArray.cpp"
#include "Actor.h"

#include "MySharedPointer.h"
#include "MySharedPointer.cpp"

namespace MyEngine{

#define WIDTH  20
#define HEIGHT  20

	class Scene
	{
		
	public:
		TemplateArray<MySharedPointer<Actor>> Actors;

		virtual void update( const double dt);

		void releaseDeadActors();

		Scene(void);
		~Scene(void);

		bool loadActorsFromConfig(const char *);
		MySharedPointer<Actor> FindActorByName(const char *);
		TemplateArray<MySharedPointer<Actor>> FindAllActorsByClass(const char *);


		// inline
		inline void addActor(MySharedPointer<Actor> &);
		inline MySharedPointer<Actor> addActor(const char *,const float width,const float height,bool is3DObj);
		inline MySharedPointer<Actor> addTriggerBox(float Bx,float By,float Bz,bool is3DObj);



	};

}
#include "Scene.inl"