#include "Director.h"
#include <iostream>
#include <thread>
#include "Actor.h"
#include "Vector3.h"
#include "conio.h"
#include "AllocationOverride.h"
#include <Windows.h>
#include "Renderer3D.h"
#include "WinTiming.h"
#include "luaplus\LuaPlus.h"


namespace MyEngine{

	
	void OnKeyPressed( unsigned int i_CharID );
	void OnKeyDown( unsigned int i_CharID );

	
	static Director *instance;
	Director* Director::sharedDirector()
	{
		if (instance)
		{
			return instance;
		}
		return NULL;
	}


	Director::~Director(void)
	{
		
		if (m_pRenderer3D)
		{
			delete m_pRenderer3D;
			m_pRenderer3D = NULL;
		}

		if(m_pRenderer2D)
		{
			delete m_pRenderer2D;
			m_pRenderer2D = NULL;
		}

		if (m_collision)
		{
			delete m_collision;
			m_collision = NULL;
		}
		if (m_pMessageManager)
		{
			delete m_pMessageManager;
			m_pMessageManager = NULL;
		}

		Cheesy::Shutdown();
	}


	// ******************initializations******************
	Director::Director(void)
	{
		init("Test",640,480,0.0166);
	}

	Director::Director(const char *i_windowName,unsigned int i_width,unsigned int i_height,double i_frameRate)
	{
		init(i_windowName,i_width,i_height,i_frameRate);
	}


	//************ pass in the Config file name to init************
	Director::Director(const char *i_pFileName)
	{
		using namespace LuaPlus;

		LuaState *pState = LuaPlus::LuaState::Create();

		if( pState )
			if (pState->DoFile(i_pFileName) == 0)
			{

				// Build the ClassTypes bits
				LuaObject Settings = pState->GetGlobal( "Settings" );
				assert( Settings.IsTable() );

				LuaObject WindowName = Settings["WindowName"];
				assert( WindowName.IsString() );

				LuaObject Width = Settings["Width"];
				assert( Width.IsNumber() );

				LuaObject Height = Settings["Height"];
				assert( Height.IsNumber() );

				LuaObject FrameRate = Settings["FrameRate"];
				assert( FrameRate.IsNumber() );

				init(WindowName.GetString(),Width.GetInteger(),Height.GetInteger(),FrameRate.GetDouble());
				
			}

		LuaState::Destroy(pState);
	
	
	}

	void Director::init(const char *i_windowName,unsigned int i_width,unsigned int i_height,double i_frameRate)
	{
		Cheesy::Create( i_windowName, i_width, i_height, true, false );

		frameRate = i_frameRate;

		currentScene = NULL;
		m_pRenderer3D = new Renderer3D();
		m_pRenderer2D = new Renderer2D();
		m_collision = new Collision();
		m_pMessageManager = new MessageManager();


		delta = 0.0f;counter = 0;
		gameState = GameState_Pause;

		instance = this;
	}






	// ******************The Main Loop******************
	void Director::run()
	{	

		Cheesy::SetKeyDownCallback(OnKeyDown);
		Cheesy::SetKeyPressCallback( OnKeyPressed );


		struct Cheesy::ColorRGBA ClearColor( 80, 80, 80, 0 );
		struct Cheesy::Point2D	 offsetBG( 0.0f, 0.0f );

		bool bQuit = false;
		
		Cheesy::Service( bQuit );

		gameState = GameState_Runing;

		double currentFrameTimeMS = Timing::WinTiming::getCurenntFrameTime_ms();
		Timing::WinTiming::setLastFrameTime_ms(currentFrameTimeMS);

		/******************3D Camera init***********************/
		D3DXVECTOR3	Eye(0.0f,0.0f,-20.0f);//( 0.0f, 4.0f, -20.0f );
		D3DXVECTOR3 LookAt( 0.0f, 0.0f, 0.0f );
		D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
		Cheesy::CreateProjection( D3DX_PI / 4, 0.1f, 100.0f );
		/***********************************************/


		while(gameState == GameState_Runing && bQuit == false)
		{
			Cheesy::Service( bQuit );

			
			
			// Get Frame Time
			currentFrameTimeMS = Timing::WinTiming::getCurenntFrameTime_ms();
					//DEBUG_PRINT("%f  \n",currentFrameTimeMS);
			delta = currentFrameTimeMS - Timing::WinTiming::getLastFrameTime_ms();

			Timing::WinTiming::setLastFrameTime_ms(currentFrameTimeMS);
			
			counter += delta;

			if(counter >= frameRate)//Frame frequency now is 1/60
			{
				counter = frameRate;

				// End the game if there is no scene/world
				if (!currentScene)
					break;

				currentScene->update(counter);

				int len = currentScene->Actors.len;

				// Project movement and Update all the game objects
				for (int i = 0 ; i < len ; i++)
				{
					MySharedPointer<Actor> &s = currentScene->Actors[i];
					if (s->getState() == ActorState_Dead)
						continue;
					s->projectMove(counter);
   					s->update(counter);
				}
				
				
				m_collision->update(counter);


				// Finalize movement
				for (int i = 0 ; i < len ; i++)
				{
					MySharedPointer<Actor> &s = currentScene->Actors[i];
					if (s->getState() == ActorState_Dead)
						continue;
					
					s->finalizeMove();

				}


				// killed actors will be marked as dead.
				currentScene->releaseDeadActors();

				m_keyPressed = 0;

				if( !bQuit )
				{
					if( Cheesy::BeginFrame( ClearColor ) )
					{
						if( Cheesy::Begin3D( ) )
						{
							Cheesy::CreateCamera( Eye, LookAt, Up );

							draw3D();
						}

						if( Cheesy::Begin2D( ) )
						{
							draw2D();
						}
						Cheesy::EndFrame();
					}
				}

				counter = 0;
			}
		}

	}

	

	bool Director::draw3D()
	{
		m_pRenderer3D->draw();
		return false;

	}

	bool Director::draw2D()
	{
		m_pRenderer2D->draw();
		return false;

	}


	void Director::pushScene(Scene* scene)
	{

		m_pRenderer3D->clear();
		m_pRenderer2D->clear();


		currentScene = scene;
		
		for (int i = 0 ; i < currentScene->Actors.len ; i++)
		{
			MySharedPointer<Actor> &s = currentScene->Actors[i];
			m_collision->addActor(s);
			if(s->is3DObj)
				m_pRenderer3D->addActor(s);
			else
				m_pRenderer2D->addActor(s);
		}
	}

	void Director::addActor(MyEngine::MySharedPointer<MyEngine::Actor> &i_Actor)
	{
		currentScene->addActor(i_Actor);
		if(i_Actor->is3DObj)
			m_pRenderer3D->addActor(i_Actor);
		else
			m_pRenderer2D->addActor(i_Actor);
		m_collision->addActor(i_Actor);

	}





	// ******************Input manager******************

	void OnKeyPressed( unsigned int i_CharID )
	{
		if (instance)
			instance->setKeyPressed( i_CharID );
	}

	void OnKeyDown( unsigned int i_CharID )
	{

		if (instance)
			instance->setKeyDown( i_CharID );
	}

	void Director::setKeyDown(const unsigned int i_CharID)
	{
		m_keyDown = i_CharID;
	}


	void Director::setKeyPressed(const unsigned int i_CharID)
	{
		m_keyPressed = i_CharID;
		m_keyDown = 999;
	}


	bool Director::isKeyDown(const unsigned int i_CharID)
	{
		if (m_keyDown == i_CharID)
			return true;
		else
			return false;
	}

	bool Director::isKeyPressed(const unsigned int i_CharID)
	{
		if (m_keyPressed == i_CharID)
			return true;
		else
			return false;
	}
}