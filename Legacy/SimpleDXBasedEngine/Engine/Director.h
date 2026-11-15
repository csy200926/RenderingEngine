#pragma once
#include "Scene.h"
#include "Renderer3D.h"
#include "Collision.h"
#include "MessageManager.h"
#include "Renderer2D.h"


namespace MyEngine{

	enum GameState{
		GameState_Runing = 0,
		GameState_Over,
		GameState_Pause
	};
	class Director
	{
	private:
		// refresher
		double frameRate;
		double delta;
		double counter;



		unsigned int m_keyDown;
		unsigned int m_keyPressed;

		MyEngine::Renderer3D *m_pRenderer3D;
		MyEngine::Renderer2D *m_pRenderer2D;
		MyEngine::Collision *m_collision;
		MyEngine::MessageManager *m_pMessageManager;
	public:

		static Director* sharedDirector();

		Scene* currentScene;

		char *renderMap;

		GameState gameState;


		bool draw2D();
		bool draw3D();
		void run();
		void pushScene(Scene* scene);

		void changeColor();

		bool isKeyDown(const unsigned int);
		void setKeyDown(const unsigned int);

		bool isKeyPressed(const unsigned int);
		void setKeyPressed(const unsigned int);


		Director(void);
		Director(const char *i_pFileName);// Load settings from files
		Director(const char *i_windowName,unsigned int i_width,unsigned int i_height,double i_frameRate);
		void init(const char *i_windowName,unsigned int i_width,unsigned int i_height,double i_frameRate);
		~Director(void);
		
		void addActor(MyEngine::MySharedPointer<MyEngine::Actor> &);
	};

}