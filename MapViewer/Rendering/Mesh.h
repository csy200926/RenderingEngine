#pragma once

#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>
#include <vector>
#include <string>
#include "../Utilities/Debugging.h"
#include "IResources.h"

namespace Rendering
{


	
	struct Vertex
	{
		glm::vec3 _position;
		glm::vec3 _normal;
		glm::vec2 _texCoords;

		void Set(glm::vec3 &position, glm::vec3 &normal, glm::vec2 &uv)
		{
			_position[0] = position.x;
			_position[1] = position.y;
			_position[2] = position.z;

			_normal[0] = normal.x;
			_normal[1] = normal.y;
			_normal[2] = normal.z;

			_texCoords[0] = uv.x;
			_texCoords[1] = uv.y;
		}
	};

	struct Texture_ID
	{
		GLuint ID;
		std::string type;
		std::string path;
	};

	class SubMesh
	{
	public:


	private:


	};

	class Material;

	class Mesh : public IResource
	{
	public :
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture_ID> textures;

		// Load mesh from this function is single mesh
		Mesh(std::string i_filePath);
		~Mesh();

		Mesh(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<Texture_ID> &textures);
		void Draw(Material *pMaterial);//Shader
		void Draw()const;
	private:
		GLuint VAO, VBO, EBO;
		void SetupMesh();

		bool useSingleVBO;
	};
	typedef std::shared_ptr<Mesh> MeshPtr;



}
