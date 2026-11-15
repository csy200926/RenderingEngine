#include "Scene.h"
#include "luaplus\LuaPlus.h"
#include "HashedString.h"


#include "Matrix.h"
#include "Vector4.h"

namespace MyEngine{



	Scene::Scene(void)
	{
		
	}
	

	Scene::~Scene(void)
	{
		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
			
			Actors.remove(Actors[i]);
		}

	
	}



	void Scene::update(const double dt)
	{

	}

	void Scene::releaseDeadActors()
	{
		
		for (int i = 0; i < Actors.len ; i++)
		{
			if (Actors[i]->getState() == ActorState_Dead)
				Actors.remove(Actors[i]);
		}
	}





	// Load info from config files
	MySharedPointer<Actor> Scene::FindActorByName(const char * i_pName)
	{
		
		for ( int i = 0; i < Actors.len ; i++)
		{
			if (Actors[i]->getName() == HashedString::Hash( i_pName ) )
				return Actors[i];
		}

		return NULL;
	}
	// Load info from config files
	TemplateArray<MySharedPointer<Actor>> Scene::FindAllActorsByClass(const char * i_pClassType)
	{
		// Hashed ClassType
		TemplateArray<MySharedPointer<Actor>>a;
		return a;
	}
	// Load info from config files
	bool Scene::loadActorsFromConfig(const char * i_pName)
	{
	
		using namespace LuaPlus;

		LuaState *pState = LuaPlus::LuaState::Create();

		if( pState )
			if (pState->DoFile(i_pName) == 0)
			{

				// Build the ClassTypes bits
				LuaObject Actors = pState->GetGlobal( "Actors" );

				for ( LuaPlus::LuaTableIterator it( Actors ); it; it.Next() )
				{
					LuaObject ThisActor = it.GetValue();
					assert( ThisActor.IsTable() );

					LuaObject Name = ThisActor["name"];
					assert( Name.IsString() );
					
					LuaObject Class = ThisActor["class"];
					assert( Class.IsString() );

					// Creating the Actors by using the information get from files
					MySharedPointer<Actor>Actor = addActor("image/game_guy.bmp",25.0,25.0,false);//25.0 default
				
					Actor->addName( Name.GetString() );
					Actor->setClassType(Class.GetString());
					
					// Make them randomly spawned to let you separate them
					float n = rand() % 100*0.01f;
					Vector3 position(200 * n,0,0);
					Actor->setPosition(position);
				
				}
				
			}
			else
			{
				
				return false;
			}
			LuaState::Destroy(pState);

			return true;
	}




}