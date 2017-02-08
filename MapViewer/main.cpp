#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>
#include "BitMap/BitMap.h"
#include "detail/func_common.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/quaternion.hpp"

#include "Utilities/Timing.h"
#include "Utilities.h"
#include "Componenets/SkyBox.h"
#include "Componenets/TextureCube.h"
#include "Componenets/Camera.h"
#include "Componenets/MeshPoints.h"
#include "Componenets/Material.h"
#include "Componenets/Mesh.h"
#include "Componenets/Texture.h"



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


#include "Componenets/InputManager.h"
#include "Componenets/GUI.h"
#include "SDL/SDL.h"

using namespace SYSR;
using namespace glm;


#pragma region static variables
GLFWwindow* window;
unsigned int ScreenHeight = 768, ScreenWidth = 1024;
bool isGameRunning = true;

#define FRONT "Images/skyBox/stormydays_ft.tga"
#define BACK "Images/skyBox/stormydays_bk.tga"
#define TOP "Images/skyBox/stormydays_up.tga"
#define BOTTOM "Images/skyBox/stormydays_dn.tga"
#define LEFT "Images/skyBox/stormydays_lf.tga"
#define RIGHT "Images/skyBox/stormydays_rt.tga"
#pragma endregion


#pragma region input Call backs

// Input helpers
bool s_wireframeMode = false;
bool s_useShadow = false;

static void mousePos_callback()//(GLFWwindow* window, double x, double y)
{
	using namespace glm;
	using namespace Rendering;
	InputManager *pInputManger = InputManager::GetInstance();

	int x = 0, y = 0;
	SDL_GetRelativeMouseState(&x,&y);
	vec2 offset(x, y);

	static const float factor = 0.001f; 
	offset *= factor;

	// get the axis to rotate around the x-axis. 
	vec3 Axis = cross(Rendering::Camera::viewVector - Rendering::Camera::cameraPos, vec3(0, 1, 0));
	// To be able to use the quaternion conjugate, the axis to
	// rotate around must be normalized.
	Axis = normalize(Axis);

	Rendering::Camera::RotateCamera(-offset.y, Axis.x, Axis.y, Axis.z);// rotate around local x axis
	Rendering::Camera::RotateCamera(-offset.x, 0, 1, 0);//rotate around local y axis

}

// hack
vec3 shadowCameraPos(232.41f, 346.67f, 13.65f);
vec3 shadowCameratarget(231.025f, 345.247f, 13.87f);

void InputUpdates()
{
	using namespace glm;
	using namespace Rendering;

	float speed = 0.1f;

	InputManager *pInputManger = InputManager::GetInstance();

	if (pInputManger->isKeyPressed(SDLK_LSHIFT))//if (GetKey(GLFW_KEY_LEFT_SHIFT))
		speed = 1.5f;
	vec3 forwarDir = Rendering::Camera::GetForwardDir();

	vec3 move(0, 0, 0);
	if (pInputManger->isKeyPressed(SDLK_w))//if (GetKey(GLFW_KEY_W))
	{
		move += Rendering::Camera::GetForwardDir() * speed;
	}
	if (pInputManger->isKeyPressed(SDLK_s))//if (GetKey(GLFW_KEY_S))
	{
		move -= Rendering::Camera::GetForwardDir() * speed;
	}
	if (pInputManger->isKeyPressed(SDLK_a))//if (GetKey(GLFW_KEY_A))
	{
		move -= Rendering::Camera::GetLeftDir() * speed;
	}
	if (pInputManger->isKeyPressed(SDLK_d))//if (GetKey(GLFW_KEY_D))
	{
		move += Rendering::Camera::GetLeftDir() * speed;
	}
	if (pInputManger->isKeyPressed(SDLK_q))//if (GetKey(GLFW_KEY_Q))
	{
		move -= vec3(0, 1, 0) * speed;
	}
	if (pInputManger->isKeyPressed(SDLK_e))//if (GetKey(GLFW_KEY_E))
	{
		move += vec3(0, 1, 0) * speed;
	}

	vec3 moveXZ(move.x,0,move.z);
	//shadowCameraPos += moveXZ;
	//shadowCameratarget += moveXZ;

	Rendering::Camera::cameraPos += move;
	Rendering::Camera::viewVector = Rendering::Camera::cameraPos + forwarDir * 10.0f;

	Rendering::Camera::Update();

	s_wireframeMode = false;
	if (pInputManger->isKeyPressed(SDLK_k))//if (GetKey(GLFW_KEY_K))
	{
		s_wireframeMode = true;
	}
	s_useShadow = false;
	if (pInputManger->isKeyPressed(SDLK_j))//if (GetKey(GLFW_KEY_K))
	{
		s_useShadow = true;
	}
	if (pInputManger->isKeyPressed(SDLK_ESCAPE))//if (GetKey(GLFW_KEY_K))
	{
		isGameRunning = false;
	}
	mousePos_callback();
}

#pragma endregion





int main(int argc, char** argv) {

	
	{
		using namespace Rendering;
		using namespace std;
		using namespace glm;

		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_Window *window = SDL_CreateWindow("STAY or TO GO", 200, 200, ScreenWidth, ScreenHeight, SDL_WINDOW_OPENGL);
		SDL_GLContext glContext = SDL_GL_CreateContext(window);
		
		// Start GL context and O/S window using the GLFW helper library
		if (!glfwInit()) {
			fprintf(stderr, "ERROR: could not start GLFW3\n");
			return 1;
		}

		// start GLEW extension handler
		glewExperimental = GL_TRUE;
		glewInit();

		system("CLS");
		printf("OpenGL version supported by this platform (%s): \n",glGetString(GL_VERSION));


		//Skybox
		TextureCube *cubeTex = NULL;
		SkyBox *skyBox = NULL;
		{
			cubeTex = new TextureCube();
			cubeTex->Init(FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT);

			skyBox = new SkyBox();
			skyBox->Init(cubeTex);
		}

		Mesh_Old* pMesh = new Mesh_Old();
		vector<Vertex_Old> vertices;
		vector<unsigned int> indices;

		Vertex_Old quadPoint;

		//Left bottom  0
		quadPoint.Set(vec3(0, 0, 0), vec3(0, 0, 1), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec2(0.0f, 0.0f));
		vertices.push_back(quadPoint);

		//Left top     1
		quadPoint.Set(vec3(0, 10, 0), vec3(0, 0, 1), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec2(0.0f, 1.0f));
		vertices.push_back(quadPoint);

		//Right top    2
		quadPoint.Set(vec3(10, 10, 0), vec3(0, 0, 1), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec2(1.0f, 1.0f));
		vertices.push_back(quadPoint);

		//Right bottom 3
		quadPoint.Set(vec3(10, 0, 0), vec3(0, 0, 1), vec4(1.0f, 1.0f, 1.0f, 1.0f), vec2(1.0f, 0.0f));
		vertices.push_back(quadPoint);

		indices.push_back(0); indices.push_back(1); indices.push_back(2);
		indices.push_back(2); indices.push_back(3); indices.push_back(0);

		pMesh->LoadFromList(vertices, indices);

		Material *pMaterial = new Material();
		pMaterial->Init("Shaders/phongVS.shader", "Shaders/phongFS.shader");
		pMesh->SetMaterial(pMaterial);

		glViewport(0, 0, ScreenWidth, ScreenHeight);
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK);

		float timer = 0.0f;
	
		using namespace  glm;
		vec3 cameraPos(0, 0, 10);
		vec3 target(0, 0, 0);
		vec3 up(0, 1, 0);

		Rendering::Camera::SetLookAt(cameraPos, target, up);
		Rendering::Camera::SetPerspective(70, 4.0f / 3.0f, 0.1f, 1000.0f);
		Utilities::Timing timing;
		timing.init(60);

		//Input stuff 
		Rendering::InputManager inputManager;
		SDL_Event inputEvent;

		SDL_SetRelativeMouseMode(SDL_TRUE);



		while (isGameRunning)//(!glfwWindowShouldClose(window))
		{
			timing.begin();

			float delta = Utilities::Timing::getDelta();
			timer += delta;

			//Will keep looping until there are no more events to process
			while (SDL_PollEvent(&inputEvent)) {
				switch (inputEvent.type) {
				case SDL_QUIT:
					isGameRunning = false;
					break;
				case SDL_MOUSEMOTION:
					inputManager.setMouseCoords((float)inputEvent.motion.x, (float)inputEvent.motion.y);
					break;
				case SDL_KEYDOWN:
					inputManager.pressKey(inputEvent.key.keysym.sym);
					break;
				case SDL_KEYUP:
					inputManager.releaseKey(inputEvent.key.keysym.sym);
					break;
				case SDL_MOUSEBUTTONDOWN:
					inputManager.pressKey(inputEvent.button.button);
					break;
				case SDL_MOUSEBUTTONUP:
					inputManager.releaseKey(inputEvent.button.button);
					break;
				}
			}
		
			// Normal GL draw methods
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



			//skyBox->Draw();

			pMesh->Draw();


			// Updates
			{


				if (timer > 0.01f)
				{
					timer = 0.0f;
					InputUpdates();
				}
			}

			
			SDL_GL_SwapWindow(window);

			timing.end();
		}



		// close GL context and any other GLFW resources
		glfwTerminate();

	
	}
	
	_CrtDumpMemoryLeaks();

	return 0;
}