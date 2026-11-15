#pragma once
#include "Material.h"
#include <glm.hpp>
#include "SkyBox.h"
#include "TextureCube.h"
#include "RenderingEngine.h"
#include "GUI.h"
namespace Rendering
{

	class RenderingEngineDiffuse : public RenderingEngine
	{

	public:

		virtual void Render();

		virtual void Initilize();
		virtual void ShutDown();

	private: 

		MaterialPtr m_screenMat;

		GLuint depthrenderBuffer;
		GLuint quad_vertexbuffer;

		GLuint framebuffer;
		GLuint renderTexture;
	};
}