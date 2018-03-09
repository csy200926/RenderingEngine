#include "RenderingEngineDiffuse.h"
#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>

#include <string>
#include <sstream>
#include <iostream>

#include "MaterialManager.h"
#include "TextureManager.h"
#include "MeshManager.h"

#include "Camera.h"

#include "../Game.h"

namespace Rendering
{

	void RenderingEngineDiffuse::Render()
	{
		//glDisable(GL_DEPTH_TEST);

		glClearColor(0.1f, 1.0f, 0.1f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // We're not using stencil buffer now

		//m_defaultMat->Activate();
		UpdateDirctLightUniform(m_defaultMat->GetProgram());

		m_pRootNode->Draw();

 	}


	void RenderingEngineDiffuse::Initilize()
	{

		m_defaultMat = MaterialManager::GetInstance()->Load("Default", "Materials/Diffuse.material");

		TexturePtr defaultTex = TextureManager::GetInstance()->Load("Default", "Images/defaultTex.jpg");
		m_defaultMat->SetTexture(defaultTex);
		//m_UIManager.init("GUI");
		//m_UIManager.loadScheme("OgreTray.scheme");
		//m_UIManager.setFont("DejaVuSans-10");
	}

	void RenderingEngineDiffuse::ShutDown()
	{

	}
}

