//#include <glew.h> // include GLEW and new version of GL on Windows
//#include <glfw3.h> // GLFW helper library
//#include <glm.hpp>
//#include <stdio.h>
//#include <vector>
//#include <map>
//#include "detail/func_common.hpp"
//#include "gtc/matrix_transform.hpp"
//#include "gtx/quaternion.hpp"
//
//#include "Utilities/Timing.h"
//#include "Utilities.h"
//#include "Componenets/SkyBox.h"
//#include "Componenets/TextureCube.h"
//#include "Componenets/Camera.h"
//#include "Componenets/MeshPoints.h"
//#include "Componenets/Material.h"
//#include "Componenets/Mesh.h"
//#include "Componenets/Model.h"
//#include "Componenets/Texture.h"
//#include "Componenets/SceneNode.h"
//
//#include "stdio.h"
//#include "stdlib.h"
//#include <time.h>
//#include <string>
//#include <fstream>
//#include <streambuf>
//#include <stdlib.h>
//#include <iostream>
//#include <thread>
//#include <chrono>
//
//#include <windows.h>
//
//#include "Componenets/InputManager.h"
//#include "Componenets/GUI.h"
//#include "SDL/SDL.h"
//
//
//using namespace glm;
//
//#pragma region static variables
//GLFWwindow* window;
//unsigned int ScreenHeight = 300, ScreenWidth = 400;
//bool isGameRunning = true;
//
//#define FRONT "Images/skyBox/stormydays_ft.tga"
//#define BACK "Images/skyBox/stormydays_bk.tga"
//#define TOP "Images/skyBox/stormydays_up.tga"
//#define BOTTOM "Images/skyBox/stormydays_dn.tga"
//#define LEFT "Images/skyBox/stormydays_lf.tga"
//#define RIGHT "Images/skyBox/stormydays_rt.tga"
//
//Rendering::Camera *s_pCamera = nullptr;
//#pragma endregion
//
//
//#pragma region input Call backs
//
//float cameraPitch = 0.0f;
//float cameraYaw = 0.0f;
//
//
//static void mousePos_callback()//(GLFWwindow* window, double x, double y)
//{
//	using namespace glm;
//	using namespace Rendering;
//	InputManager *pInputManger = InputManager::GetInstance();
//
//	int x = 0, y = 0;
//	SDL_GetRelativeMouseState(&x, &y);
//	vec2 offset(x, -y);
//
//	static const float factor = 0.1f;
//	offset *= factor;
//
//	cameraYaw += offset.x;
//	cameraPitch += offset.y;
//	
//
//	if (cameraPitch > 89.0f)
//		cameraPitch = 89.0f;
//	if (cameraPitch < -89.0f)
//		cameraPitch = -89.0f;
//
//	glm::vec3 front;
//	front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
//	front.y = sin(glm::radians(cameraPitch));
//	front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
//
//	s_pCamera->viewVector = glm::normalize(front);
//
//
//}
//
//
//void InputUpdates()
//{
//
//	using namespace glm;
//	using namespace Rendering;
//
//	float speed = 0.1f;
//
//	InputManager *pInputManger = InputManager::GetInstance();
//
//	if (pInputManger->isKeyPressed(SDLK_LSHIFT))//if (GetKey(GLFW_KEY_LEFT_SHIFT))
//		speed = 1.5f;
//	vec3 forwarDir = s_pCamera->GetForwardDir();
//
//	vec3 move(0, 0, 0);
//	if (pInputManger->isKeyPressed(SDLK_w))//if (GetKey(GLFW_KEY_W))
//	{
//		move += s_pCamera->GetForwardDir() * speed;
//	}
//	if (pInputManger->isKeyPressed(SDLK_s))//if (GetKey(GLFW_KEY_S))
//	{
//		move -= s_pCamera->GetForwardDir() * speed;
//	}
//	if (pInputManger->isKeyPressed(SDLK_a))//if (GetKey(GLFW_KEY_A))
//	{
//		move -= s_pCamera->GetLeftDir() * speed;
//	}
//	if (pInputManger->isKeyPressed(SDLK_d))//if (GetKey(GLFW_KEY_D))
//	{
//		move += s_pCamera->GetLeftDir() * speed;
//	}
//
//	if (pInputManger->isKeyPressed(SDLK_q))//if (GetKey(GLFW_KEY_Q))
//	{
//		move -= vec3(0, 1, 0) * speed;
//	}
//	if (pInputManger->isKeyPressed(SDLK_e))//if (GetKey(GLFW_KEY_E))
//	{
//		move += vec3(0, 1, 0) * speed;
//	}
//
//	vec3 moveXZ(move.x, 0, move.z);
//
//	s_pCamera->cameraPos += move;
//	//s_pCamera->viewVector = forwarDir;// s_pCamera->cameraPos + forwarDir * 10.0f;
//
//	
//	if (pInputManger->isKeyPressed(SDLK_ESCAPE))//if (GetKey(GLFW_KEY_K))
//	{
//		isGameRunning = false;
//	}
//
//
//	mousePos_callback();
//
//	s_pCamera->Update();
//}
//
//#pragma endregion
//
//
//
//
//
//int main(int argc, char** argv) {
//
//
//	{
//		using namespace Rendering;
//		using namespace std;
//		using namespace glm;
//
//		SDL_Init(SDL_INIT_EVERYTHING);
//		SDL_Window *window = SDL_CreateWindow("STAY or TO GO", 200, 200, ScreenWidth, ScreenHeight, SDL_WINDOW_OPENGL);
//		SDL_GLContext glContext = SDL_GL_CreateContext(window);
//
//		// Start GL context and O/S window using the GLFW helper library
//		if (!glfwInit()) {
//			fprintf(stderr, "ERROR: could not start GLFW3\n");
//			return 1;
//		}
//
//		// start GLEW extension handler
//		glewExperimental = GL_TRUE;
//		glewInit();
//
//		system("CLS");
//		printf("OpenGL version supported by this platform (%s): \n", glGetString(GL_VERSION));
//
//
//		s_pCamera = new Rendering::Camera();
//
//		//Skybox
//		TextureCube *cubeTex = NULL;
//		SkyBox *skyBox = NULL;
//		{
//			cubeTex = new TextureCube();
//			cubeTex->Init(FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT);
//
//			skyBox = new SkyBox();
//			skyBox->Init(cubeTex);
//		}
//
//
//		Material *pMaterial = new Material();
//		pMaterial->Init("Shaders/DefaultVS.shader", "Shaders/DefaultFS.shader");
//
//		Model *pModel = new Model("Models/test.obj");
//		pModel->SetMaterial(pMaterial);
//		
//		SceneNode *pRoot = new SceneNode();
//		pRoot->AttachRenderable(pModel);
//
//		Model *pModel_2 = new Model("Models/test.obj");
//		pModel_2->SetMaterial(pMaterial);
//
//		SceneNode *pTestNode = new SceneNode();
//		pTestNode->AttachRenderable(pModel_2);
//
//		pRoot->AddChild(pTestNode);
//		pTestNode->m_position = vec3(80,0,0);
//		pTestNode->m_scale = vec3(0.5f,0.5f,0.5f);
//
//		glViewport(0, 0, ScreenWidth, ScreenHeight);
//		glEnable(GL_DEPTH_TEST);
//
//		float timer = 0.0f;
//
//		using namespace  glm;
//		vec3 cameraPos(0, 0, 10);
//		vec3 target(0, 0, 0);
//		vec3 up(0, 1, 0);
//
//		s_pCamera->SetLookAt(cameraPos, target, up);
//		s_pCamera->SetPerspective(70, 4.0f / 3.0f, 0.1f, 1000.0f);
//		Utilities::Timing timing;
//		timing.init(60);
//
//		//Input stuff 
//		Rendering::InputManager inputManager;
//		SDL_Event inputEvent;
//
//		SDL_SetRelativeMouseMode(SDL_TRUE);
//
//		while (isGameRunning)//(!glfwWindowShouldClose(window))
//		{
//			timing.begin();
//
//			float delta = Utilities::Timing::getDelta();
//			timer += delta;
//
//			//Will keep looping until there are no more events to process
//			while (SDL_PollEvent(&inputEvent)) {
//				switch (inputEvent.type) {
//				case SDL_QUIT:
//					isGameRunning = false;
//					break;
//				case SDL_MOUSEMOTION:
//					inputManager.setMouseCoords((float)inputEvent.motion.x, (float)inputEvent.motion.y);
//					break;
//				case SDL_KEYDOWN:
//					inputManager.pressKey(inputEvent.key.keysym.sym);
//					break;
//				case SDL_KEYUP:
//					inputManager.releaseKey(inputEvent.key.keysym.sym);
//					break;
//				case SDL_MOUSEBUTTONDOWN:
//					inputManager.pressKey(inputEvent.button.button);
//					break;
//				case SDL_MOUSEBUTTONUP:
//					inputManager.releaseKey(inputEvent.button.button);
//					break;
//				}
//			}
//
//			// Normal GL draw methods
//			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//			skyBox->Draw();
//			pRoot->Pitch(0.0003f); pRoot->Yaw(0.0003f);
//			pRoot->Draw();
//
//			// Updates
//			{
//
//
//				if (timer > 0.01f)
//				{
//					timer = 0.0f;
//					InputUpdates();
//				}
//			}
//
//
//			SDL_GL_SwapWindow(window);
//
//			timing.end();
//		}
//
//
//
//		// close GL context and any other GLFW resources
//		glfwTerminate();
//
//
//	}
//
//	_CrtDumpMemoryLeaks();
//
//	return 0;
//}
#include "Game.h"
int main(int argc, char** argv)
{
	Game myGame;

	myGame.Initilize(800, 600);
	myGame.StartRunning();
	myGame.ShutDown();

	return 0;
}