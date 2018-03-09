#include "Material.h"
#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>
#include <iostream>

#include "TextureManager.h"
#include "Texture.h"
#include "Camera.h"

#include <iostream>
#include <fstream>

#include "LuaPlus.h"

namespace Rendering
{
	Material::Material(const std::string &i_path,int version)
	{

		
		using namespace LuaPlus;
		LuaStateOwner luaState;
		const int luaResult = luaState->DoFile(i_path.c_str());

		LuaObject matObj = luaState->GetGlobal("Material");
		LuaObject propertiesObj = matObj["Properties"];
		LuaObject texturesObj = matObj["Textures"];
		LuaObject ShadersObj = matObj["Shaders"];

		// Iterate property table and Set uniforms
		for (LuaTableIterator it(propertiesObj); it; it.Next())
		{
			LuaObject &obj = it.GetValue();
			const char* uniformName = it.GetKey().GetString();

			// Check if is color. Alpha is not considered yet
			if (obj.IsTable() && obj.GetCount() == 3)
			{
				// Index start from 1
				float r = obj[1].ToNumber();
				float g = obj[2].ToNumber();
				float b = obj[3].ToNumber();

				m_vector3Map[uniformName] = glm::vec3(r,g,b);
			}
			else if (obj.IsNumber())
			{
				m_floatMap[uniformName] = obj.ToNumber();
			}
			else{}// Invalid type
		}

		// Iterate texture table
		for (LuaTableIterator it(texturesObj); it; it.Next())
		{
			LuaObject &obj = it.GetValue();
			const char* uniformName = it.GetKey().GetString();

			//TODO: now just assuming all texture path is correct
			TexturePtr tex = TextureManager::GetInstance()->Load(uniformName, obj.ToString());
			m_textureMap[uniformName] = tex;
		}

		// Iterate Shaders table
		LuaObject vertexShd = ShadersObj["VertexShader"];
		LuaObject pixelShd = ShadersObj["PixelShader"];

		Init(vertexShd.ToString(),pixelShd.ToString());
		



	}

	Material::Material()
	{
		m_type = RT_Material;
	}


	Material::~Material()
	{
		int debug = 0;
	}

	bool Material::Init(const char *i_vs, const char * i_fs)
	{

		// Load and compile the vertex and fragment shaders
		GLuint vertexShader = load_and_compile_shader(i_vs, GL_VERTEX_SHADER);
		GLuint fragmentShader = load_and_compile_shader(i_fs, GL_FRAGMENT_SHADER);

		// Attach the above shader to a program
		m_program = glCreateProgram();
		glAttachShader(m_program, vertexShader);
		glAttachShader(m_program, fragmentShader);

		// Flag the shaders for deletion
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		// Link and use the program
		glLinkProgram(m_program);
		glUseProgram(m_program);
		m_MMatrixLocation = glGetUniformLocation(m_program, "ModelToWorld_Matrix");
		m_PMatrixLocation = glGetUniformLocation(m_program, "Projective_Matrix");
		m_VMatrixLocation = glGetUniformLocation(m_program, "WorldToView_Matrix");
		
//		glUseProgram(m_program);

		return true;
	}

	bool Material::ShowDown()
	{
		if (m_program)
			glDeleteProgram(m_program);

		return true;
	}

	void Material::Activate()
	{
		glUseProgram(m_program);

		//for (auto const &it_vec3 : m_vector3Map)
		//{
		//	glUniform3f(glGetUniformLocation(m_program, it_vec3.first.c_str()),
		//		it_vec3.second.r,
		//		it_vec3.second.g,
		//		it_vec3.second.b);
		//}

		//for (auto const &it_float : m_floatMap)
		//{
		//	glUniform1f(glGetUniformLocation(m_program, it_float.first.c_str()), it_float.second);
		//}

		//int texIndex = 0;
		//for (auto const &it_tex : m_textureMap)
		//{
		//	glActiveTexture(GL_TEXTURE0 + texIndex);
		//	it_tex.second->Bind();
		//	texIndex++;
		//}

		glUniformMatrix4fv(m_PMatrixLocation, 1, GL_FALSE, &Camera::Projective_Matrix[0][0]);
		glUniformMatrix4fv(m_VMatrixLocation, 1, GL_FALSE, &Camera::WorldToView_Matrix[0][0]);
		glUniformMatrix4fv(m_MMatrixLocation, 1, GL_FALSE, &Camera::ModelToWorld_Matrix[0][0]);

		if (m_texture.get() != NULL)
			m_texture->Activate();


	}

	void Material::SetTexture(TexturePtr i_texture)
	{
		m_texture = i_texture;

		// tell shader which uniform finds which slot
		GLint tex_loc = glGetUniformLocation(m_program, "basic_texture");
		glUniform1i(tex_loc, 0);
	}


	GLuint Material::load_and_compile_shader(const char *fname, GLenum shaderType)
	{
		// Load a shader from an external file
		std::vector<char> buffer;
		read_shader_src(fname, buffer);
		const char *src = &buffer[0];

		// Compile the shader
		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &src, NULL);
		glCompileShader(shader);
		// Check the result of the compilation
		GLint test;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
		if (!test) {
			std::cerr << "Shader compilation failed with this message:" << std::endl;
			std::vector<char> compilation_log(512);
			glGetShaderInfoLog(shader, compilation_log.size(), NULL, &compilation_log[0]);
			std::cerr << &compilation_log[0] << std::endl;
			glfwTerminate();
			//exit(-1);
			system("pause");
		}
		return shader;

	}

	void Material::read_shader_src(const char *fname, std::vector<char> &buffer)
	{
		std::ifstream in;
		in.open(fname, std::ios::binary);

		if (in.is_open()) {
			// Get the number of bytes stored in this file
			in.seekg(0, std::ios::end);
			size_t length = (size_t)in.tellg();

			// Go to start of the file
			in.seekg(0, std::ios::beg);

			// Read the content of the file in a buffer
			buffer.resize(length + 1);
			in.read(&buffer[0], length);
			in.close();
			// Add a valid C - string end
			buffer[length] = '\0';
		}
		else {
			std::cerr << "Unable to open " << fname << " I'm out!" << std::endl;
			system("pause");
			exit(-1);
		}
	}


}