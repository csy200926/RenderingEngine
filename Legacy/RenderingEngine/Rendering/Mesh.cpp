#include "Mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>


#include "Material.h"
#include "Camera.h"

#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//#define POINT_MODE

namespace Rendering 
{


	//------------------------------------------------------------------------------------------------------------
	Mesh::Mesh(std::string i_filePath)
	{
		using namespace std;

		// Read
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(i_filePath,aiProcess_OptimizeMeshes|  aiProcess_OptimizeGraph | aiProcess_Triangulate | aiProcess_FlipUVs);
		// Check for errors
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
			return;
		}

		using namespace std;

		int indexStart = 0;
		for (int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
		{
			const aiMesh* mesh = scene->mMeshes[meshIndex];
			indexStart = m_vertices.size();

			for (GLuint i = 0; i < mesh->mNumVertices; i++)
			{
				Vertex vertex;
				glm::vec3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
				// Positions
				vector.x = mesh->mVertices[i].x;
				vector.y = mesh->mVertices[i].y;
				vector.z = mesh->mVertices[i].z;
				vertex._position = vector;
				// Normals
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex._normal = vector;
				// Texture Coordinates
				if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
				{
					glm::vec2 vec;
					// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
					// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
					vec.x = mesh->mTextureCoords[0][i].x;
					vec.y = mesh->mTextureCoords[0][i].y;
					vertex._texCoords = vec;
				}
				else
					vertex._texCoords = glm::vec2(0.0f, 0.0f);
				m_vertices.push_back(vertex);

			}


			std::vector<GLuint> indices;
			for (GLuint i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				// Retrieve all indices of the face and store them in the indices vector
				for (GLuint j = 0; j < face.mNumIndices; j++)
					indices.push_back(face.mIndices[j] + indexStart);
			}

			SubMesh *pSubMesh = new SubMesh(indices);
			m_subMeshes.push_back(pSubMesh);
		}

		SetupMesh();

	}

	Mesh::Mesh(std::vector<Vertex> &vertices, std::vector<GLuint> &indices)
	{

	}


	void Mesh::SetupMesh()
	{
		glGenVertexArrays(1,&VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER,VBO);
		glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex) , &m_vertices[0],GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _texCoords)));

		glBindVertexArray(0);
	}
	void Mesh::Draw_Pre()const
	{
		glBindVertexArray(VAO);
		// xxxx?
	}

	void Mesh::Draw_Post()const
	{
		glBindVertexArray(0);
		// xxxx?
	}

	void Mesh::Draw(int submeshIndex) const
	{
		if (submeshIndex < m_subMeshes.size())
			m_subMeshes[submeshIndex]->Draw();
	}

	void Mesh::Draw() const
	{
		for (int subMeshIndex = 0; subMeshIndex < m_subMeshes.size(); subMeshIndex++)
		{
			SubMesh* pSubMesh = m_subMeshes[subMeshIndex];
			pSubMesh->Draw();
		}
	}

	Mesh::~Mesh()
	{
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &VAO);

		for (int i = 0; i < m_subMeshes.size(); i++)
		{
			delete m_subMeshes[i];
		}
	}


	SubMesh::SubMesh(std::vector<GLuint> &i_indices)
	{
		m_indices = i_indices;

		glGenBuffers(1, &EBO);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size()*sizeof(GLuint), &m_indices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void SubMesh::Draw()
	{
		using namespace std;
		using namespace glm;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glDrawElements(
			GL_TRIANGLES,      // mode
			m_indices.size(),
			GL_UNSIGNED_INT,   // type
			0          // element array buffer offset
			);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

}