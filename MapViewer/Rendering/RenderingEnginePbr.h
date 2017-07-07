#pragma once
#include "Lighting.h"
#include "SceneNode.h"
#include "ISingleton.h"
#include <vector>
#include "Material.h"
#include <glm.hpp>
#include "SkyBox.h"
#include "TextureCube.h"
#include "RenderingEngine.h"
namespace Rendering
{



	class RenderingEnginePbr : public RenderingEngine
	{

	public:

		virtual void Render();
	
		virtual void Initilize();
		virtual void ShutDown();
	protected:

		MaterialPtr m_defaultMat;
		MaterialPtr m_screenMat;

		SkyBox m_skyBox;
		TextureCube m_cubeTex;

		GLuint framebuffer;
		GLuint renderTexture;
		GLuint depthrenderBuffer;
		GLuint quad_vertexbuffer;

		// AA use
		GLuint multiTex;
		GLuint framebuffer_AA;

		// PBR
		MaterialPtr m_PBRcon;
		GLuint m_envCubemap;

	};
}