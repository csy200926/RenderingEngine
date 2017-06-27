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

	void RenderingEngine::Render()
	{
		//glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);

		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // We're not using stencil buffer now
		//glEnable(GL_DEPTH_TEST);
		//m_deferred_first->Activate();
	
		//m_pRootNode->Draw();

		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//GLint shaderID = m_deferred_second->GetProgram();

		//glUseProgram(shaderID);
		//GLint texLoc = 0;
		//texLoc = glGetUniformLocation(shaderID, "gPosition");
		//glUniform1i(texLoc, 0);
		//texLoc = glGetUniformLocation(shaderID, "gNormal");
		//glUniform1i(texLoc, 1);
		//texLoc = glGetUniformLocation(shaderID, "gAlbedoSpec");
		//glUniform1i(texLoc, 2);
		//
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, gPosition);
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, gNormal); 
		//glActiveTexture(GL_TEXTURE2);
		//glBindTexture(GL_TEXTURE_2D, gColorSpec);

		//UpdatePointLightUniform(m_deferred_second.get());
		//UpdateDirctLightUniform(m_deferred_second.get());

		//glEnableVertexAttribArray(0);
		//glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		//glVertexAttribPointer(
		//	0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		//	3,                  // size
		//	GL_FLOAT,           // type
		//	GL_FALSE,           // normalized?
		//	0,                  // stride
		//	(void*)0            // array buffer offset
		//	);

		//// Draw the triangles !
		//glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
		//glDisableVertexAttribArray(0);
		
		// First pass
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_AA);

		glClearColor(0.1f, 1.0f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // We're not using stencil buffer now
		glEnable(GL_DEPTH_TEST);

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
		//m_skyBox.DrawBigCubeAndMat();
		m_skyBox.Draw();

		// Bind PBR cubemap
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
		m_defaultMat->Activate();
		UpdateDirctLightUniform(m_defaultMat.get());
		UpdatePointLightUniform(m_defaultMat.get());

		m_pRootNode->Draw();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_AA);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
		glBlitFramebuffer(0, 0, Game::screenWidth, Game::screenHeight, 0, 0, Game::screenWidth, Game::screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT ); // We're not using stencil buffer now
		glDisable(GL_DEPTH_TEST);

		glUseProgram( m_screenMat->GetProgram() );
		glBindTexture(GL_TEXTURE_2D, renderTexture);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// Draw the triangles !
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
		glDisableVertexAttribArray(0);
		



	}



	void RenderingEngine::UpdatePointLightUniform(Material *i_pMaterial)
	{


		using namespace std;

		// Params needed
		const int MAX_POINT_LIGHTS = 5;
		GLint program = i_pMaterial->GetProgram();
		int shaderLightIndex = 0;

		glUniform3f(glGetUniformLocation(program, "albedo"), 1.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(program, "ao"), 1.0f);
		glUniform1f(glGetUniformLocation(program, "metallic"), 0.5f);
		glUniform1f(glGetUniformLocation(program, "roughness"), 0.2f);


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
		TexturePtr defaultTex = TextureManager::GetInstance()->Load("Default", "Images/defaultTex.jpg");

		m_defaultMat = MaterialManager::GetInstance()->Load("Default", "Materials/Default.material");
		m_defaultMat->SetTexture(defaultTex);

		MeshManager::GetInstance()->Load("Default", "Models/Maskboy.FBX");

		using namespace std;

		const int MAX_POINT_LIGHTS = 5;
		GLint program = m_defaultMat->GetProgram();
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			std::stringstream lightIndexSS;
			lightIndexSS << "pointLights[" << i << "].";
			string lightIndexStr = lightIndexSS.str();
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "position").c_str()), 999, 999,999);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "diffuse").c_str()), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(program, (lightIndexStr + "specular").c_str()), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "constant").c_str()), 1.0f);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "linear").c_str()), 0.09f);
			glUniform1f(glGetUniformLocation(program, (lightIndexStr + "quadratic").c_str()), 0.032f);

		}

		m_pDirectionalLight = m_pRootNode->AddComponent<DirectionalLight>();
		m_pDirectionalLight->m_diffuse = vec3(1.0f, 1.0f, 0.5f);

		//Skybox init
		m_cubeTex.Init(FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT);
		m_skyBox.Init(&m_cubeTex);


		m_screenMat = MaterialManager::GetInstance()->Load("Screen", "Materials/Screen.material");
		// Render texture init
		{
			framebuffer = 0;
			glGenFramebuffers(1, &framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

			renderTexture = 0;
			glGenTextures(1, &renderTexture);
			glBindTexture(GL_TEXTURE_2D, renderTexture);

			// Set data as empty
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Game::screenWidth, Game::screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTexture, 0);


			// If depth test is needed
			depthrenderBuffer = 0;
			glGenRenderbuffers(1, &depthrenderBuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthrenderBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Game::screenWidth, Game::screenHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderBuffer);

			// which one should be activate, similar to glActiveTexture(GL_TEXTURE0);
			GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, drawBuffers);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// Init quad
		{
			// The fullscreen quad's FBO
			static const GLfloat g_quad_vertex_buffer_data[] = {
				-1.0f, -1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
				1.0f, 1.0f, 0.0f,
			};


			glGenBuffers(1, &quad_vertexbuffer);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
		}

		//MSAA
		{
			framebuffer_AA = 0;
			glGenFramebuffers(1, &framebuffer_AA);
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_AA);

			//Bind multiTex to frameBuffer_AA
			glGenTextures(1, &multiTex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multiTex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, Game::screenWidth, Game::screenHeight, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multiTex, 0);

			GLuint rbo;
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, Game::screenWidth, Game::screenHeight);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}


		// pbr: setup cubemap to render to and attach to framebuffer
		// ---------------------------------------------------------
		m_envCubemap;
		glGenTextures(1, &m_envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		// ----------------------------------------------------------------------------------------------
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 20.0f);
		glm::mat4 captureViews[] =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
		};

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		
		
		// pbr: setup framebuffer
		// ----------------------
		unsigned int captureFBO;
		unsigned int captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);


		// pbr: convolution cube map 
		{
			m_PBRcon = MaterialManager::GetInstance()->Load("Convolution", "Materials/PBRconvolution.material");
			glViewport(0, 0, 32,32);
			glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
			glUseProgram(m_PBRcon->GetProgram());
			m_skyBox.UseTexCube();

			glUniformMatrix4fv(glGetUniformLocation(m_PBRcon->GetProgram(), "Projective_Matrix"), 1, GL_FALSE, &captureProjection[0][0]);
			for (unsigned int i = 0; i < 6; ++i)
			{
				glUniformMatrix4fv(glGetUniformLocation(m_PBRcon->GetProgram(), "WorldToView_Matrix"), 1, GL_FALSE, &captureViews[i][0][0]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubemap, 0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				m_skyBox.DrawBigCube();
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		glViewport(0, 0, Game::screenWidth, Game::screenHeight);

		return;
		// Deferred
		glGenFramebuffers(1, &m_gBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
		

		glGenTextures(1, &gPosition);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Game::screenWidth, Game::screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Game::screenWidth, Game::screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

		glGenTextures(1, &gColorSpec);
		glBindTexture(GL_TEXTURE_2D, gColorSpec);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Game::screenWidth, Game::screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

		GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, attachments);

		unsigned int rboDepth;
		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Game::screenWidth, Game::screenHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
		// finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);



		m_deferred_first = MaterialManager::GetInstance()->Load("Deferred_first", "Materials/Deferred_first.material");
		m_deferred_second = MaterialManager::GetInstance()->Load("Deferred_second", "Materials/Deferred_second.material");

	}

	void RenderingEngine::ShutDown()
	{

	}

	void RenderingEngine::OnRenderImage(TexturePtr scr, TexturePtr tar, MaterialPtr material)
	{

	}



}

