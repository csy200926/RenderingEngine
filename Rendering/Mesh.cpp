#include "Mesh.h"
#include "Terrain.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include "AssetIOSystem.h"
#include "TextureManager.h"
#include "MaterialManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <map>
namespace Rendering
{
	static TexturePtr LoadMaterialTexture(const aiScene &i_scene, const aiMaterial &i_material, aiTextureType i_type,
										  const std::filesystem::path &i_directory, unsigned &o_missing,
										  std::vector<std::string> &o_issues,
										  std::vector<std::filesystem::path> &o_dependencies)
	{
		aiString name;
		if (i_material.GetTexture(i_type, 0, &name) != AI_SUCCESS || !name.length)
			return nullptr;
		std::string value = name.C_Str();
		std::replace(value.begin(), value.end(), '\\', '/');
		if (i_scene.GetEmbeddedTexture(value.c_str()))
		{
			auto issue = "embedded image is not supported by the OBJ/MTL image path: " + value;
			o_issues.push_back(issue);
			Platform::Log("WARNING: " + issue);
			++o_missing;
			return nullptr;
		}
		auto path = std::filesystem::u8path(value);
		if (!path.is_absolute())
			path = i_directory / path;
		if (!std::filesystem::is_regular_file(path))
			path = i_directory / std::filesystem::u8path(value).filename();
		if (!std::filesystem::is_regular_file(path))
		{
			auto issue = "missing model texture: " + value + " (resolved as " + path.u8string() + ")";
			o_issues.push_back(issue);
			Platform::Log("WARNING: " + issue);
			++o_missing;
			return nullptr;
		}
		o_dependencies.push_back(path.lexically_normal());
		auto relative = path.lexically_relative(Platform::GetAssetPath(""));
		return TextureManager::GetInstance()->Load(relative.generic_u8string(), relative.generic_u8string());
	}
	static void RecordObjMaterialLibraries(const std::filesystem::path &i_path, std::vector<std::string> &o_issues,
										   std::vector<std::filesystem::path> &o_dependencies)
	{
		// 第一步：只检查 OBJ 明确声明的 mtllib，不把导入器尝试的候选路径误当真实依赖。
		auto extension = i_path.extension().u8string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
					   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (extension != ".obj")
			return;
		auto text = Platform::ReadText(i_path);
		if (text.compare(0, 3, "\xEF\xBB\xBF") == 0)
			text.erase(0, 3);
		std::istringstream source(text);
		std::string line;
		while (std::getline(source, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			auto start = line.find_first_not_of(" \t");
			if (start == std::string::npos || line.compare(start, 6, "mtllib") != 0 ||
				(start + 6 < line.size() && line[start + 6] != ' ' && line[start + 6] != '\t'))
				continue;
			auto nameStart = line.find_first_not_of(" \t", start + 6);
			if (nameStart == std::string::npos)
			{
				o_issues.push_back("OBJ declares an empty mtllib");
				continue;
			}
			// 与当前 Assimp OBJ 解析一致，mtllib 的剩余整行是一个文件名，可包含空格。
			auto name = line.substr(nameStart);
			std::replace(name.begin(), name.end(), '\\', '/');
			auto dependency = std::filesystem::u8path(name);
			if (!dependency.is_absolute())
				dependency = i_path.parent_path() / dependency;
			dependency = dependency.lexically_normal();
			o_dependencies.push_back(dependency);
			if (!std::filesystem::is_regular_file(dependency))
				o_issues.push_back("missing declared OBJ material library: " + dependency.u8string());
		}
	}
	static std::map<std::string, std::string> ReadMaterialFiles(
		const std::vector<std::filesystem::path> &i_dependencies)
	{
		std::map<std::string, std::string> result;
		auto root = std::filesystem::canonical(Platform::GetAssetPath(""));
		for (const auto &library : i_dependencies)
		{
			if (library.extension() != ".mtl" || !std::filesystem::is_regular_file(library))
				continue;
			std::istringstream source(Platform::ReadText(library));
			std::string line;
			while (std::getline(source, line))
			{
				// 普通 MTL 工具仍把这一行当注释；本引擎用它找完整材质，避免透明度重复相乘。
				std::istringstream fields(line);
				std::string comment, marker, name, file;
				if (!(fields >> comment >> marker) || comment != "#" || marker != "RendererMaterial")
					continue;
				if (!(fields >> name))
					throw std::runtime_error("MTL material reference is missing its material name: " +
											 library.u8string());
				std::getline(fields >> std::ws, file);
				auto end = file.find_last_not_of(" \t\r");
				if (end == std::string::npos)
					throw std::runtime_error("MTL material reference has an empty path: " + library.u8string());
				file.resize(end + 1);
				Platform::ToWide(file);
				auto relative = std::filesystem::u8path(file);
				if (relative.is_absolute() || relative.has_root_name() || file.find(':') != std::string::npos ||
					file.find('\0') != std::string::npos)
					throw std::runtime_error("MTL material reference must be relative: " + file);
				auto full = std::filesystem::weakly_canonical(library.parent_path() / relative);
				auto local = full.lexically_relative(root);
				if (local.empty() || local.is_absolute() || *local.begin() == ".." || full.extension() != ".material" ||
					!std::filesystem::is_regular_file(full))
					throw std::runtime_error("MTL material reference is missing or outside Assets: " + file);
				if (!result.emplace(name, local.generic_u8string()).second)
					throw std::runtime_error("Duplicate MTL material reference: " + name);
			}
		}
		return result;
	}
	Mesh::Mesh(const std::string &i_path) : IResource(RT_Mesh)
	{
		auto start = std::chrono::steady_clock::now();
		m_fileName = i_path;
		auto path = Platform::GetAssetPath(i_path);
		if (path.extension() == ".terrain")
		{
			LoadTerrain(*this, path);
			return;
		}
		RecordObjMaterialLibraries(path, m_importIssues, m_importDependencies);
		Assimp::Importer importer;
		importer.SetIOHandler(new AssetIOSystem(path.parent_path(), &m_importDependencies));
		auto *pScene =
			importer.ReadFile(path.generic_u8string(),
							  aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_GenSmoothNormals |
								  aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices | aiProcess_FlipUVs);
		if (!pScene || !pScene->HasMeshes())
			throw std::runtime_error("Model import failed: " + i_path + ": " + importer.GetErrorString());
		auto materialFiles = ReadMaterialFiles(m_importDependencies);
		unsigned missing = 0, textured = 0, masked = 0, normalMapped = 0;
		for (unsigned i = 0; i < pScene->mNumMaterials; ++i)
		{
			auto material = std::make_shared<Material>();
			auto &source = *pScene->mMaterials[i];
			aiString materialName;
			source.Get(AI_MATKEY_NAME, materialName);
			auto reference = materialFiles.find(materialName.C_Str());
			if (reference != materialFiles.end())
			{
				// 先用完整材质，MTL 的 d / map_Kd 不再叠加一次；直接添加模型与加载整场景一致。
				material = MaterialManager::GetInstance()->Load(reference->second, reference->second);
				m_importDependencies.push_back(Platform::GetAssetPath(reference->second));
				if (material->m_texture)
					++textured;
				if (material->m_normalTexture)
					++normalMapped;
				if (material->m_alphaCutoff > 0)
					++masked;
				m_materials.push_back(material);
				continue;
			}
			aiColor3D color(1, 1, 1);
			source.Get(AI_MATKEY_COLOR_DIFFUSE, color);
			float opacity = 1, shininess = 0;
			source.Get(AI_MATKEY_OPACITY, opacity);
			source.Get(AI_MATKEY_SHININESS, shininess);
			material->m_color = glm::vec4(color.r, color.g, color.b, glm::clamp(opacity, 0.f, 1.f));
			material->m_metallic = 0;
			material->m_roughness = glm::clamp(std::sqrt(2.f / (shininess + 2.f)), .15f, 1.f);
			material->m_texture = LoadMaterialTexture(*pScene, source, aiTextureType_DIFFUSE, path.parent_path(),
													  missing, m_importIssues, m_importDependencies);
			material->m_opacityTexture = LoadMaterialTexture(*pScene, source, aiTextureType_OPACITY, path.parent_path(),
															 missing, m_importIssues, m_importDependencies);
			material->m_normalTexture = LoadMaterialTexture(*pScene, source, aiTextureType_NORMALS, path.parent_path(),
															missing, m_importIssues, m_importDependencies);
			if (!material->m_normalTexture)
			{
				for (auto type : {aiTextureType_HEIGHT, aiTextureType_DISPLACEMENT})
				{
					aiString normalPath;
					if (source.GetTexture(type, 0, &normalPath) == AI_SUCCESS)
					{
						std::string name = normalPath.C_Str();
						if (name.find("_ddn") != std::string::npos || name.find("_normal") != std::string::npos)
							material->m_normalTexture =
								LoadMaterialTexture(*pScene, source, type, path.parent_path(), missing, m_importIssues,
													m_importDependencies);
					}
				}
			}
			if (material->m_texture)
				++textured;
			if (material->m_normalTexture)
				++normalMapped;
			material->m_transparent = material->m_color.a < .999f;
			if (!material->m_transparent &&
				(material->m_opacityTexture || (material->m_texture && material->m_texture->m_hasAlpha)))
			{
				material->m_alphaCutoff = .5f;
				++masked;
			}
			m_materials.push_back(material);
		}
		size_t vertices = 0, indices = 0;
		for (unsigned i = 0; i < pScene->mNumMeshes; ++i)
		{
			vertices += pScene->mMeshes[i]->mNumVertices;
			indices += static_cast<size_t>(pScene->mMeshes[i]->mNumFaces) * 3;
		}
		m_vertices.reserve(vertices);
		m_indices.reserve(indices);
		m_subMeshes.reserve(pScene->mNumMeshes);
		for (unsigned m = 0; m < pScene->mNumMeshes; ++m)
		{
			auto *pMesh = pScene->mMeshes[m];
			if (!(pMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
				continue;
			if (!pMesh->HasPositions() || !pMesh->HasNormals())
				throw std::runtime_error("Triangle mesh is missing positions or normals: " + i_path);
			unsigned offset = static_cast<unsigned>(m_vertices.size());
			SubMesh part;
			part.m_firstIndex = static_cast<unsigned>(m_indices.size());
			part.m_materialIndex = pMesh->mMaterialIndex;
			part.m_minimum = glm::vec3(1e30f);
			part.m_maximum = glm::vec3(-1e30f);
			for (unsigned v = 0; v < pMesh->mNumVertices; ++v)
			{
				auto p = pMesh->mVertices[v], n = pMesh->mNormals[v];
				auto uv = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][v] : aiVector3D();
				m_vertices.push_back({{p.x, p.y, p.z}, {n.x, n.y, n.z}, {uv.x, uv.y}});
				part.m_minimum = glm::min(part.m_minimum, glm::vec3(p.x, p.y, p.z));
				part.m_maximum = glm::max(part.m_maximum, glm::vec3(p.x, p.y, p.z));
			}
			for (unsigned f = 0; f < pMesh->mNumFaces; ++f)
			{
				auto &triangle = pMesh->mFaces[f];
				if (triangle.mNumIndices != 3)
					continue;
				for (unsigned i = 0; i < 3; ++i)
					m_indices.push_back(offset + triangle.mIndices[i]);
			}
			part.m_indexCount = static_cast<unsigned>(m_indices.size()) - part.m_firstIndex;
			m_subMeshes.push_back(part);
		}
		if (m_vertices.empty() || m_indices.empty())
			throw std::runtime_error("Model contains no triangles: " + i_path);
		Platform::Log("Mesh imported: " + i_path + ", vertices=" + std::to_string(m_vertices.size()) + ", triangles=" +
					  std::to_string(m_indices.size() / 3) + ", submeshes=" + std::to_string(m_subMeshes.size()) +
					  ", materials=" + std::to_string(m_materials.size()) + ", textured=" + std::to_string(textured) +
					  ", masked=" + std::to_string(masked) + ", normal-mapped=" + std::to_string(normalMapped) +
					  ", missing-textures=" + std::to_string(missing) + ", import-ms=" +
					  std::to_string(
						  std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()));
	}
	Mesh::Mesh(const std::vector<Vertex> &i_vertices, const std::vector<unsigned> &i_indices)
		: IResource(RT_Mesh), m_vertices(i_vertices), m_indices(i_indices)
	{
		m_subMeshes.push_back({0, static_cast<unsigned>(m_indices.size()), 0});
		// 程序生成的方块/平面也需要真实包围盒，视口拾取、聚焦和透明排序都会用到。
		if (!m_vertices.empty())
		{
			auto &part = m_subMeshes.back();
			part.m_minimum = part.m_maximum = m_vertices.front()._position;
			for (const auto &vertex : m_vertices)
			{
				part.m_minimum = glm::min(part.m_minimum, vertex._position);
				part.m_maximum = glm::max(part.m_maximum, vertex._position);
			}
		}
	}
	MeshPtr Mesh::CreateCube()
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned> indices;
		glm::vec3 normals[] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
		glm::vec3 corners[][4] = {
			{{1, -1, -1}, {1, -1, 1}, {1, 1, 1}, {1, 1, -1}}, {{-1, -1, 1}, {-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}},
			{{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}}, {{-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1}},
			{{1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}}, {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}}};
		glm::vec2 uv[] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
		for (int f = 0; f < 6; ++f)
		{
			unsigned offset = static_cast<unsigned>(vertices.size());
			for (int i = 0; i < 4; ++i)
				vertices.push_back({corners[f][i] * .5f, normals[f], uv[i]});
			// 旧方块的绕序朝内，这里统一朝外，与法线和其他基础几何体保持一致。
			for (auto index : {0u, 2u, 1u, 0u, 3u, 2u})
				indices.push_back(offset + index);
		}
		auto mesh = std::make_shared<Mesh>(vertices, indices);
		// 程序网格没有磁盘文件，用稳定标识恢复同一种几何体。
		mesh->m_fileName = "builtin:cube";
		return mesh;
	}
	MeshPtr Mesh::CreateQuad()
	{
		auto mesh = std::make_shared<Mesh>(std::vector<Vertex>{{{-.5f, -.5f, 0}, {0, 0, 1}, {0, 1}},
															   {{.5f, -.5f, 0}, {0, 0, 1}, {1, 1}},
															   {{.5f, .5f, 0}, {0, 0, 1}, {1, 0}},
															   {{-.5f, .5f, 0}, {0, 0, 1}, {0, 0}}},
										   std::vector<unsigned>{0, 1, 2, 0, 2, 3});
		mesh->m_fileName = "builtin:quad";
		return mesh;
	}
} // namespace Rendering
