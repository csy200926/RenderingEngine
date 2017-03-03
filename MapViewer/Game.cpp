#include "Game.h"

int Game::screenWidth;
int Game::screenHeight;

void Game::Initilize(int i_screenWidth, int i_screenHeight)
{
	screenWidth = i_screenWidth;
	screenHeight = i_screenHeight;

	using namespace Rendering;
	using namespace std;
	using namespace glm;

	SDL_Init(SDL_INIT_EVERYTHING);
	m_window = SDL_CreateWindow("STAY or TO GO", 200, 200, i_screenWidth, i_screenHeight, SDL_WINDOW_OPENGL);
	SDL_GLContext glContext = SDL_GL_CreateContext(m_window);

	// Start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return;
	}

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	printf("OpenGL version supported by this platform (%s): \n", glGetString(GL_VERSION));

	glViewport(0, 0, i_screenWidth, i_screenHeight);
	glEnable(GL_DEPTH_TEST);

	m_pRootNode = new SceneNode();

	m_timing.init(60);

	vec3 cameraPos(0, 0, 10);
	vec3 target(0, 0, 0);
	vec3 up(0, 1, 0);

	m_camera.SetLookAt(cameraPos, target, up);
	m_camera.SetPerspective(70, i_screenWidth / i_screenHeight, 0.1f, 1000.0f);

	m_pInputManager = InputManager::CreateInstance();
	m_pMaterialManager = MaterialManager::CreateInstance();
	m_pTextureManager = TextureManager::CreateInstance();
	m_pMeshManager = MeshManager::CreateInstance();

	OnStart();
}


void Game::Running()
{

	SDL_Event inputEvent;

	while (m_isGameRunning)
	{
		m_timing.begin();// Utilities::Timing::getDelta();

		//Will keep looping until there are no more events to process
		while (SDL_PollEvent(&inputEvent)) {
			switch (inputEvent.type) {
			case SDL_QUIT:
				m_isGameRunning = false;
				break;
			case SDL_MOUSEMOTION:
				m_pInputManager->setMouseCoords((float)inputEvent.motion.x, (float)inputEvent.motion.y);
				break;
			case SDL_KEYDOWN:
				m_pInputManager->pressKey(inputEvent.key.keysym.sym);
				break;
			case SDL_KEYUP:
				m_pInputManager->releaseKey(inputEvent.key.keysym.sym);
				break;
			case SDL_MOUSEBUTTONDOWN:
				m_pInputManager->pressKey(inputEvent.button.button);
				break;
			case SDL_MOUSEBUTTONUP:
				m_pInputManager->releaseKey(inputEvent.button.button);
				break;
			}
		}

		// Logic update
		Update();

		// Normal GL draw methods
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_skyBox.Draw();

		m_pRootNode->Draw();

		SDL_GL_SwapWindow(m_window);

		m_timing.end();


	}
}

void Game::ShutDown()
{

	InputManager::DestroyInstance();
	MaterialManager::DestroyInstance();
	TextureManager::DestroyInstance();
	MeshManager::DestroyInstance();

	delete m_pRootNode;

	OnDestroy();
	glfwTerminate();
}




void Game::OnStart()
{

	m_cubeTex.Init(FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT);
	m_skyBox.Init(&m_cubeTex);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	cameraPitch = 0.0f;
	cameraYaw = 0.0f;
}

void Game::Update()
{
	using namespace glm;
	using namespace Rendering;

	float speed = 0.1f;


	if (m_pInputManager->isKeyPressed(SDLK_LSHIFT))//if (GetKey(GLFW_KEY_LEFT_SHIFT))
		speed = 1.5f;
	vec3 forwarDir = m_camera.GetForwardDir();

	vec3 move(0, 0, 0);
	if (m_pInputManager->isKeyPressed(SDLK_w))//if (GetKey(GLFW_KEY_W))
	{
		move += m_camera.GetForwardDir() * speed;
	}
	if (m_pInputManager->isKeyPressed(SDLK_s))//if (GetKey(GLFW_KEY_S))
	{
		move -= m_camera.GetForwardDir() * speed;
	}
	if (m_pInputManager->isKeyPressed(SDLK_a))//if (GetKey(GLFW_KEY_A))
	{
		move -= m_camera.GetLeftDir() * speed;
	}
	if (m_pInputManager->isKeyPressed(SDLK_d))//if (GetKey(GLFW_KEY_D))
	{
		move += m_camera.GetLeftDir() * speed;
	}

	if (m_pInputManager->isKeyPressed(SDLK_q))//if (GetKey(GLFW_KEY_Q))
	{
		move -= vec3(0, 1, 0) * speed;
	}
	if (m_pInputManager->isKeyPressed(SDLK_e))//if (GetKey(GLFW_KEY_E))
	{
		move += vec3(0, 1, 0) * speed;
	}

	vec3 moveXZ(move.x, 0, move.z);

	m_camera.cameraPos += move;

	if (m_pInputManager->isKeyPressed(SDLK_ESCAPE))//if (GetKey(GLFW_KEY_K))
	{
		m_isGameRunning = false;
	}

	{
		InputManager *pInputManger = InputManager::GetInstance();

		int x = 0, y = 0;
		SDL_GetRelativeMouseState(&x, &y);
		vec2 offset(x, -y);

		static const float factor = 0.1f;
		offset *= factor;

		cameraYaw += offset.x;
		cameraPitch += offset.y;


		if (cameraPitch > 89.0f)
			cameraPitch = 89.0f;
		if (cameraPitch < -89.0f)
			cameraPitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
		front.y = sin(glm::radians(cameraPitch));
		front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));

		m_camera.viewVector = glm::normalize(front);
	}

	m_camera.Update();

	m_pRootNode->InternalUpdate();
}

void Game::OnDestroy()
{


}

