#include "Material.h"
#include <string>
#include <fstream>
#include <streambuf>
#include <stdlib.h>
#include <iostream>

#include "Texture.h"
#include "Camera.h"

#include <iostream>
#include <fstream>

namespace Rendering
{
	Material::Material(const std::string &path)
	{
		m_texture = TexturePtr(nullptr);

		using namespace std;

		ifstream file(path);

		if (!file)
		{
			return;
		}

		// 0 vertex shader   1 pixel shader
		// 2 3 4 ?? textures??
		int index = 0;
		const int vertexShaderPathIndex = 0;
		const int fragmentShaderPathIndex = 1;

		string vertShdPath;
		string pixelShdPath;

		std::string s;
		while (std::getline(file, s))
		{
			if (index == vertexShaderPathIndex)
			{
				vertShdPath = s;
			}
			else if (index == fragmentShaderPathIndex)
			{
				pixelShdPath = s;
			}
			index++;
		}

		Init(vertShdPath.c_str(), pixelShdPath.c_str());

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

//#pragma region Lights
//		GLint viewPosLoc = glGetUniformLocation(m_program, "viewPos");
//
//		Camera *pCurrentCamera = Rendering::Camera::GetInstance();
//		glUniform3f(viewPosLoc, pCurrentCamera->m_position.x, pCurrentCamera->m_position.y, pCurrentCamera->m_position.z);
//
//		// Directional light
//		glUniform3f(glGetUniformLocation(m_program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
//		glUniform3f(glGetUniformLocation(m_program, "dirLight.ambient"), 0.05f, 0.05f, 0.05f);
//		glUniform3f(glGetUniformLocation(m_program, "dirLight.diffuse"), 0.4f, 0.4f, 0.4f);
//		glUniform3f(glGetUniformLocation(m_program, "dirLight.specular"), 0.5f, 0.5f, 0.5f);
//
//		glUniform1f(glGetUniformLocation(m_program, "NR_POINT_LIGHTS"), 2.0);
//		// Point light 1
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[0].position"), 40, 1, 0);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.8f);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[0].specular"), 1.0f, 1.0f, 1.0f);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[0].constant"), 1.0f);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[0].linear"), 0.09);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[0].quadratic"), 0.032);
//
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[1].position"), 10, 1, 0);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[1].diffuse"), 0.8f, 0.8f, 0.8f);
//		glUniform3f(glGetUniformLocation(m_program, "pointLights[1].specular"), 1.0f, 1.0f, 1.0f);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[1].constant"), 1.0f);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[1].linear"), 0.09);
//		glUniform1f(glGetUniformLocation(m_program, "pointLights[1].quadratic"), 0.032);
//#pragma endregion


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
			exit(-1);
		}
	}


}