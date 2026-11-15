#pragma once
#include "IResources.h"
#include "Material.h"
#include <glm.hpp>
#include <filesystem>
#include <vector>
namespace Rendering
{
	struct TerrainData;
	struct Vertex
	{
		glm::vec3 _position;
		glm::vec3 _normal;
		glm::vec2 _texCoords;
	};
	struct SubMesh
	{
		unsigned m_firstIndex = 0, m_indexCount = 0, m_materialIndex = 0;
		glm::vec3 m_minimum = glm::vec3(0), m_maximum = glm::vec3(0);
	};
	class Mesh : public IResource
	{
	  public:
		explicit Mesh(const std::string &i_path);
		Mesh(const std::vector<Vertex> &i_vertices, const std::vector<unsigned> &i_indices);
		static std::shared_ptr<Mesh> CreateTerrain(unsigned i_resolution = 129, float i_size = 100);
		std::shared_ptr<Mesh> CloneTerrain() const;
		void SetTerrainFile(const std::string &i_path);
		std::shared_ptr<TerrainData> m_terrain;
		uint64_t m_revision = 1;
		int GetSubmeshCount() const { return static_cast<int>(m_subMeshes.size()); }
		static std::shared_ptr<Mesh> CreateCube();
		static std::shared_ptr<Mesh> CreateQuad();
		// 内建几何体不用磁盘模型文件，保存场景时只需记住 builtin:xxx。
		static const std::vector<std::string> &GetBuiltinNames();
		static std::shared_ptr<Mesh> CreateBuiltin(const std::string &i_name);
		std::vector<Vertex> m_vertices;
		std::vector<unsigned> m_indices;
		std::vector<SubMesh> m_subMeshes;
		std::vector<MaterialPtr> m_materials;
		// 宽容导入时保留诊断和真实依赖；场景文件据此执行更严格的可移植性校验。
		std::vector<std::string> m_importIssues;
		std::vector<std::filesystem::path> m_importDependencies;
	};
	using MeshPtr = std::shared_ptr<Mesh>;
} // namespace Rendering
