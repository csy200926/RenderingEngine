#pragma once
#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include <stdio.h>
#include <vector>
#include <map>
#include "detail/func_common.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"

#include "Utilities/Timing.h"

#include "Rendering/Camera.h"
#include "Rendering/MeshPoints.h"
#include "Rendering/Material.h"
#include "Rendering/Mesh.h"
#include "Rendering/Texture.h"
#include "Rendering/SceneNode.h"
#include "Rendering/MaterialManager.h"
#include "Rendering/TextureManager.h"
#include "Rendering/MeshManager.h"
#include "Rendering/RenderingEngine.h"
#include "Rendering/RenderingEnginePbr.h"
#include "Rendering/RenderingEngineDeferred.h"

#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>

#include <windows.h>

#include "Rendering/InputManager.h"
#include "Rendering/GUI.h"
#include "SDL/SDL.h"

using namespace glm;
using namespace Rendering;




class Game
{
public:

	static int screenWidth;
	static int screenHeight;

	void Initilize(int i_screenWidth,int i_screenHeight);
	void StartRunning(){Running();}
	void ShutDown();

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


private:

	RenderingEngine* m_pRenderingEngine;

	SDL_Window* m_window;

	Utilities::Timing m_timing;

	bool m_isGameRunning = true;

	void Running();
};