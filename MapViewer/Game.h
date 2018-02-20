#pragma once

#include "Utilities/Timing.h"

#include "Rendering/Camera.h"

#include "Rendering/SceneNode.h"

#include "Rendering/RenderingEngine.h"


#include "Rendering/GUI.h"


using namespace Rendering;


namespace Rendering
{
	class InputManager;
	class MaterialManager;
	class TextureManager;
	class MeshManager;
}

class Game
{
	
public:

	static int screenWidth;
	static int screenHeight;

	void Initilize(int i_screenWidth,int i_screenHeight);

	void StartRunning(){Running();}
	void ShutDown();

	void RenderFrame();
	void UpdateFrame();

protected:

	virtual void OnStart();
	virtual void OnDestroy();
	virtual void Update();

	InputManager* m_pInputManager;
	SceneNode* m_pRootNode;

	MaterialManager *m_pMaterialManager;
	TextureManager * m_pTextureManager;
	MeshManager * m_pMeshManager;


	Camera m_camera;
	float cameraPitch;
	float cameraYaw;

	void SaveScene();
	void LoadScene(const char * i_pPath);
private:

	RenderingEngine* m_pRenderingEngine;

	SDL_Window* m_window;

	Utilities::Timing m_timing;

	bool m_isGameRunning = true;

	void Running();
};