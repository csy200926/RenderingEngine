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

namespace Rendering
{

	void RenderingEngine::Render()
	{
		UpdateDirctLightUniform(m_defaultMat.get());
		UpdatePointLightUniform(m_defaultMat.get());
		m_pRootNode->Draw();
	}



	void RenderingEngine::UpdatePointLightUniform(Material *i_pMaterial)
	{


		using namespace std;

		// Params needed
		const int MAX_POINT_LIGHTS = 5;
		GLint program = i_pMaterial->GetProgram();
		int shaderLightIndex = 0;

		// m_pointLights size no bigger than MAX_POINT_LIGHTS
		for (int lightIndex = 0; shaderLightIndex < MAX_POINT_LIGHTS && lightIndex < m_pPointLights.size(); lightIndex++)
		{
			LightBase *pLight = m_pPointLights[lightIndex];

			if (pLight->IsActivated() == false)
				continue;

			std::stringstream lightIndexSS;
			lightIndexSS << "pointLights[" << shaderLightIndex << "].";
			string lightIndexStr = lightIndexSS.str();

			glm::vec3 position = pLight->GetPosition();
			glm::vec3 ambient = pLight->m_ambient;
			glm::vec3 diffuse = pLight->m_diffuse;
			glm::vec3 specular = pLight->m_specular;

			float constant = pLight->m_constant;
			float linear = pLight->m_linear;
			float quadratic = pLight->m_quadratic;

			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), position.x, position.y, position.z);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "ambient").c_str()), ambient.x, ambient.y, ambient.z);
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
		glm::vec3 ambient = m_pDirectionalLight->m_ambient;
		glm::vec3 diffuse = m_pDirectionalLight->m_diffuse;
		glm::vec3 specular = m_pDirectionalLight->m_specular;
		glm::vec3 direction = m_pDirectionalLight->GetDirection();

		glUniform3f(glGetUniformLocation(program, "dirLight.direction"), direction.x, direction.y, direction.z);
		glUniform3f(glGetUniformLocation(program, "dirLight.ambient"), ambient.x, ambient.y, ambient.z);
		glUniform3f(glGetUniformLocation(program, "dirLight.diffuse"), diffuse.x, diffuse.y, diffuse.z);
		glUniform3f(glGetUniformLocation(program, "dirLight.specular"), specular.x, specular.y, specular.z);
	}

	void RenderingEngine::Initilize()
	{

		m_defaultMat = MaterialManager::GetInstance()->Load("Default", "Materials/Default.material");
		TexturePtr defaultTex = TextureManager::GetInstance()->Load("Default", "Images/defaultTex.jpg");
		m_defaultMat->SetTexture(defaultTex);

		using namespace std;

		const int MAX_POINT_LIGHTS = 5;
		GLint program = m_defaultMat->GetProgram();
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			std::stringstream lightIndexSS;
			lightIndexSS << "pointLights[" << i << "].";
			string lightIndexStr = lightIndexSS.str();
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), 0, 0, 0);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "ambient").c_str()), 0.05f, 0.05f, 0.05f);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "diffuse").c_str()), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "specular").c_str()), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "constant").c_str()), 0);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "linear").c_str()), 0);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "quadratic").c_str()), 0);

		}

		m_pDirectionalLight = m_pRootNode->AddComponent<DirectionalLight>();
	}

	void RenderingEngine::ShutDown()
	{

	}



}

