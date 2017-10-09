#include "RenderingEngineDeferred.h"
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

	void RenderingEngineDeferred::Render()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // We're not using stencil buffer now
		glEnable(GL_DEPTH_TEST);
		m_deferred_first->Activate();
	
		m_pRootNode->Draw();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLint shaderID = m_deferred_second->GetProgram();

		glUseProgram(shaderID);
		GLint texLoc = 0;
		texLoc = glGetUniformLocation(shaderID, "gPosition");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(shaderID, "gNormal");
		glUniform1i(texLoc, 1);
		texLoc = glGetUniformLocation(shaderID, "gAlbedoSpec");
		glUniform1i(texLoc, 2);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal); 
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gColorSpec);

		UpdatePointLightUniform(m_deferred_second.get());
		UpdateDirctLightUniform(m_deferred_second.get());

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


	void RenderingEngineDeferred::Initilize()
	{
		using namespace glm;
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

	void RenderingEngineDeferred::ShutDown()
	{

	}



}

