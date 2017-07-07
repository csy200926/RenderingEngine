#include "RenderingEngine.h"
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
	RenderingEngine* RenderingEngine::s_pRenderingEngine = nullptr;

	void RenderingEngine::Render()
	{
		
	}

	void RenderingEngine::UpdatePointLightUniform(Material *i_pMaterial)
	{


		using namespace std;

		// Params needed
		const int MAX_POINT_LIGHTS = 5;
		GLint program = i_pMaterial->GetProgram();
		int shaderLightIndex = 0;


		// m_pointLights size no bigger than MAX_POINT_LIGHTS
		for (int lightIndex = 0; lightIndex < MAX_POINT_LIGHTS; lightIndex++)
		{

			std::stringstream lightIndexSS;
			lightIndexSS << "pointLights[" << lightIndex << "].";
			string lightIndexStr = lightIndexSS.str();

			if (lightIndex >= m_pPointLights.size())
			{
				glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), 999, 999, 999);
				continue;
			}

			LightBase *pLight = m_pPointLights[shaderLightIndex];
			if (pLight->IsActivated() == false)
			{
				glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), 999, 999, 999);
				continue;
			}


			glm::vec3 position = pLight->GetPosition();
			glm::vec3 diffuse = pLight->m_diffuse;
			glm::vec3 specular = pLight->m_specular;

			float constant = pLight->m_constant;
			float linear = pLight->m_linear;
			float quadratic = pLight->m_quadratic;
			
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), position.x, position.y, position.z);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "diffuse").c_str()), diffuse.x, diffuse.y, diffuse.z);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "specular").c_str()), specular.x, specular.y, specular.z);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "constant").c_str()), constant);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "linear").c_str()), linear);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "quadratic").c_str()), quadratic);

			shaderLightIndex++;
		}
	}

	void RenderingEngine::UpdateDirctLightUniform(Material *i_pMaterial)
	{
		GLint program = i_pMaterial->GetProgram();

		GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
		Camera *pCurrentCamera = Rendering::Camera::GetInstance();
		glUniform3f(viewPosLoc, pCurrentCamera->cameraPos.x, pCurrentCamera->cameraPos.y, pCurrentCamera->cameraPos.z);

		glm::vec3 position = m_pDirectionalLight->GetPosition();
		glm::vec3 diffuse = m_pDirectionalLight->m_diffuse;
		glm::vec3 specular = m_pDirectionalLight->m_specular;
		glm::vec3 direction = m_pDirectionalLight->GetDirection();

		glUniform3f(glGetUniformLocation(program, "dirLight.direction"), direction.x, direction.y, direction.z);
		glUniform3f(glGetUniformLocation(program, "dirLight.diffuse"), diffuse.x, diffuse.y, diffuse.z);
		glUniform3f(glGetUniformLocation(program, "dirLight.specular"), specular.x, specular.y, specular.z);
	}

	void RenderingEngine::Initilize()
	{

	}

	void RenderingEngine::ShutDown()
	{

	}

	RenderingEngine::RenderingEngine()
	{
		s_pRenderingEngine = this;
	}



}

