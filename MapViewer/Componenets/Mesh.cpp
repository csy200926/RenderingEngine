#include "Mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>


#include "Material.h"
#include "Camera.h"

#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"

//#define POINT_MODE

namespace Rendering 
{
	Mesh_Old::Mesh_Old()
	{

		m_position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		m_orientation = glm::quat(glm::vec3(0,0,0));
	}
	Mesh_Old::~Mesh_Old()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &m_buffer);
		glDeleteBuffers(1, &m_indexBuffer);;
	}
	void Mesh_Old::Draw()
	{
		using namespace glm;

		glBindVertexArray(vao);
		mat4 scale = glm::scale(mat4(1.0f), m_scale);
		mat4 rotation = glm::mat4_cast(m_orientation);
		mat4 translation = glm::translate(m_position);

		Camera::ModelToWorld_Matrix = translation * rotation * scale;

		m_material->Activate();

		// put texture to slot 0  ||GL_TEXTURE0 - 0,  GL_TEXTURE1 - 1, GL_TEXTURE2 - 2....
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, m_tempTexture);
#ifdef POINT_MODE
		glDrawArrays(GL_POINTS, 0, m_vertexCount);
#else
		glDrawElements(
			GL_TRIANGLES,      // mode
			m_indexCount,
			GL_UNSIGNED_INT,   // type
			0          // element array buffer offset
			);
#endif

		glBindVertexArray(0);
	}


	void Mesh_Old::LoadFromList(std::vector<Vertex_Old> &i_vertices, std::vector<unsigned int>&i_indices)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		using namespace std;

		// Load vertices
		{
			m_vertexCount = i_vertices.size();
			Vertex_Old *vertices = i_vertices.data();

			glGenBuffers(1, &m_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
			glBufferData(GL_ARRAY_BUFFER,
				m_vertexCount*sizeof(Vertex_Old),
				&vertices[0],
				GL_STATIC_DRAW);

			// Specify 
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);

			//use layout to tell the shader set location in order
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Old), reinterpret_cast<void *>(offsetof(Vertex_Old, _position)));
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_Old), reinterpret_cast<void *>(offsetof(Vertex_Old, _uv)));
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_Old), reinterpret_cast<void *>(offsetof(Vertex_Old, _color)));
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Old), reinterpret_cast<void *>(offsetof(Vertex_Old, _normal)));
		}

		// Load indices
		{
			m_indexCount = i_indices.size();
			GLuint* indices = i_indices.data();

			// Generate a buffer for the indices
			glGenBuffers(1, &m_indexBuffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
			
		}

		glBindVertexArray(0);
		
	}


	//------------------------------------------------------------------------------------------------------------

	Mesh::Mesh(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, std::vector<Texture_ID> &textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		SetupMesh();
	}


	void Mesh::SetupMesh()
	{
		glGenVertexArrays(1,&VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER,VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex) , &vertices[0],GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _position)));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _normal)));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, _texCoords)));

		glBindVertexArray(0);
	}

	void Mesh::Draw(Material *pMaterial)
	{
		using namespace std;
		using namespace glm;

		GLuint diffuseNr = 1;
		GLuint specularNr = 1;
		for (GLuint i = 0; i < this->textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			std::string name = this->textures[i].type;
			std::string number = (name == "texture_diffuse") ? std::to_string(diffuseNr++) : std::to_string(specularNr++);

			glUniform1i(glGetUniformLocation(pMaterial->GetProgram(), ("material." + name + number).c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].ID);
		}

		glBindVertexArray(VAO);
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),
			GL_UNSIGNED_INT,   // type
			0          // element array buffer offset
			);
		glBindVertexArray(0);
	}

}