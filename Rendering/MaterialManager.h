#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Material.h"
namespace Rendering
{
	class MaterialManager : public IResourceManager, public Singleton<MaterialManager>
	{
	  public:
		MaterialManager() { m_nameResMap["Default"] = std::make_shared<Material>(); }
		MaterialPtr Create(const std::string &i_name)
		{
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end())
				return std::static_pointer_cast<Material>(it->second);
			auto material = std::make_shared<Material>();
			m_nameResMap[i_name] = material;
			return material;
		}
		MaterialPtr Load(const std::string &i_name, const std::string &i_path)
		{
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end() && it->second->GetFileName() == i_path)
				return std::static_pointer_cast<Material>(it->second);
			auto material = std::make_shared<Material>(i_path);
			m_nameResMap[i_name] = material;
			return material;
		}
		MaterialPtr GetByName(const std::string &i_name) const
		{
			return std::static_pointer_cast<Material>(GetResourceByName(i_name));
		}
	};
} // namespace Rendering
