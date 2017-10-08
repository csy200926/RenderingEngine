#include "RenderingEnginePbr.h"
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

	void RenderingEnginePbr::Render()
	{
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


	void RenderingEnginePbr::Initilize()
	{
		TexturePtr defaultTex = TextureManager::GetInstance()->Load("Default", "Images/defaultTex.jpg");

		m_defaultMat = MaterialManager::GetInstance()->Load("Default", "Materials/Default.material");
		m_defaultMat->SetTexture(defaultTex);

		

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

	}

	void RenderingEnginePbr::ShutDown()
	{

	}



}

