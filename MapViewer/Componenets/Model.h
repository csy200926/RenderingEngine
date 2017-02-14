#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <glew.h> // include GLEW and new version of GL on Windows
#include <glfw3.h> // GLFW helper library
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Material.h"
#include "Camera.h"

namespace Rendering
{
	class Model
	{
	public:
		glm::vec3 m_position;
		glm::vec3 m_scale;
		glm::quat m_orientation;

		/*  Functions   */
		Model(GLchar* path)
		{
			m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
			m_orientation = glm::quat(glm::vec3(0, 0, 0));

			LoadModel(path);
		}
		void Draw(Material *pMaterial)
		{
			using namespace glm;

			mat4 scale = glm::scale(mat4(1.0f), m_scale);
			mat4 rotation = glm::mat4_cast(m_orientation);
			mat4 translation = glm::translate(m_position);

			Camera::ModelToWorld_Matrix = translation * rotation * scale;

			pMaterial->Activate();

			for (GLuint i = 0; i < meshes.size(); i++)
				meshes[i].Draw(pMaterial);
		};
	private:
		/*  Model Data  */
		std::vector<Mesh> meshes;
		std::string directory;

		std::vector<Texture_ID>textures_loaded;

		/*  Functions   */
		void LoadModel(std::string path);;

		void ProcessNode(aiNode* node, const aiScene* scene);
		Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

		std::vector<Texture_ID> LoadMaterialTextures(aiMaterial* mat, aiTextureType type,std::string typeName);

		GLint TextureFromFile(const char* path, std::string directory);
	};

}
