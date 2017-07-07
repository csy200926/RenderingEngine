#pragma once

#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include "IResources.h"
namespace Rendering
{


	class Texture : public IResource
	{
	private:


	public:
		GLuint m_textureID;

		Texture();
		~Texture();

		bool virtual LoadFromPath(const char *i_path);
		bool virtual ShowDown();

		void virtual Activate();
		void virtual Bind();

	};



	typedef std::shared_ptr<Texture> TexturePtr;
}
