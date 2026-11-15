#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Mesh.h"
namespace Rendering
{
	class MeshManager : public IResourceManager, public Singleton<MeshManager>
	{
	  public:
		MeshManager()
		{
			m_nameResMap["Default"] = Mesh::CreateCube();
			m_nameResMap["Quad"] = Mesh::CreateQuad();
		}
		MeshPtr Load(const std::string &i_name, const std::string &i_path)
		{
			if (std::filesystem::u8path(i_path).extension() == ".terrain")
			{
				// 地形由场景和撤销历史持有；旧场景关闭后不把所有快照永久留在全局缓存。
				for (auto it = m_terrainMeshes.begin(); it != m_terrainMeshes.end();)
					if (it->second.expired())
						it = m_terrainMeshes.erase(it);
					else
						++it;
				auto found = m_terrainMeshes.find(i_name);
				if (found != m_terrainMeshes.end())
					if (auto mesh = found->second.lock())
						if (mesh->GetFileName() == i_path)
							return mesh;
				auto mesh = std::make_shared<Mesh>(i_path);
				m_terrainMeshes[i_name] = mesh;
				return mesh;
			}
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end() && it->second->GetFileName() == i_path)
				return std::static_pointer_cast<Mesh>(it->second);
			auto mesh = std::make_shared<Mesh>(i_path);
			m_nameResMap[i_name] = mesh;
			return mesh;
		}
		MeshPtr GetByName(const std::string &i_name) const
		{
			auto found = m_terrainMeshes.find(i_name);
			if (found != m_terrainMeshes.end())
				if (auto mesh = found->second.lock())
					return mesh;
			return std::static_pointer_cast<Mesh>(GetResourceByName(i_name));
		}
		MeshPtr GetBuiltin(const std::string &i_name)
		{
			// 同一种几何体只生成一次；场景里放多个球时共享网格，各自保存 Transform。
			if (i_name == "builtin:cube")
				return GetByName("Default");
			if (i_name == "builtin:quad")
				return GetByName("Quad");
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end())
				return std::static_pointer_cast<Mesh>(it->second);
			auto mesh = Mesh::CreateBuiltin(i_name);
			m_nameResMap[i_name] = mesh;
			return mesh;
		}

	  private:
		std::map<std::string, std::weak_ptr<Mesh>> m_terrainMeshes;
	};
} // namespace Rendering
