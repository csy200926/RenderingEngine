#pragma once
#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include <vector>
#include "IResources.h"
#include <memory>
#include "Texture.h"

namespace Rendering
{



	class Material : public IResource
	{
	private:

		GLuint load_and_compile_shader(const char *fname, GLenum shaderType);
		void read_shader_src(const char *fname, std::vector<char> &buffer);

		TexturePtr m_texture;
	public:
		GLint m_VMatrixLocation;
		GLint m_MMatrixLocation;
		GLint m_PMatrixLocation;

		// Hack
		GLint m_DepthMVPLocation;

		GLint GetProgram(){ return m_program; };

		bool Init(const char *i_vs, const char * i_fs);
		bool ShowDown();

		void SetTexture(TexturePtr i_texture);

		Material(const std::string &path);
		~Material();

		GLint m_program;

		void Activate();
		void Use(){	glUseProgram(m_program);};
	};

	typedef std::shared_ptr<Material> MaterialPtr;
}

