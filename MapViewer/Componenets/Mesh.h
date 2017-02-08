#pragma once

#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>
#include <vector>
#include <string>
#include "../Utilities/Debugging.h"


namespace Rendering
{


	struct Vertex_Old
	{

		GLfloat _position[3];
		GLfloat _normal[3];
		GLfloat _uv[2];
		GLfloat _color[4];

		Vertex_Old()
		{
		}

		Vertex_Old(glm::vec3 &position, glm::vec3 &normal, glm::vec4 &color, glm::vec2 &uv)
		{
			_position[0] = position.x;
			_position[1] = position.y;
			_position[2] = position.z;

			_normal[0] = normal.x;
			_normal[1] = normal.y;
			_normal[2] = normal.z;

			_color[0] = color.x;
			_color[1] = color.y;
			_color[2] = color.z;
			_color[3] = color.w;

			_uv[0] = uv.x;
			_uv[1] = uv.y;
		}

		void Set(glm::vec3 &position, glm::vec3 &normal, glm::vec4 &color, glm::vec2 &uv)
		{
			_position[0] = position.x;
			_position[1] = position.y;
			_position[2] = position.z;

			_normal[0] = normal.x;
			_normal[1] = normal.y;
			_normal[2] = normal.z;

			_color[0] = color.x;
			_color[1] = color.y;
			_color[2] = color.z;
			_color[3] = color.w;

			_uv[0] = uv.x;
			_uv[1] = uv.y;
		}
	};
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};

	struct Texture_ID
	{
		GLuint ID;
		std::string type;
		std::string path;
	};


	class Material;

	class Mesh
	{
	public :
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture_ID> textures;


		Mesh(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<Texture_ID> &textures);
		void Draw(Material *pMaterial);//Shader
	private:
		GLuint VAO, VBO, EBO;
		void SetupMesh();
	};



	class Mesh_Old
	{
	public:
		

		GLuint vao;

		glm::vec3 m_position;
		glm::vec3 m_scale;
		glm::quat m_orientation;


		void LoadFromList(std::vector<Vertex_Old> &i_vertices, std::vector<unsigned int>&i_indices);
		void Draw();

		inline void SetMaterial(Material *i_pMaterial){ m_material = i_pMaterial; };

		Mesh_Old();
		~Mesh_Old();
	private:
		Material *m_material;

		GLuint	m_buffer;
		GLuint  m_indexBuffer;

		unsigned int m_indexCount;
		unsigned int m_vertexCount;
	};

}
