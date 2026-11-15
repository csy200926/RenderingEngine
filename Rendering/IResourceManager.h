#pragma once
#include "IResources.h"
#include <map>
namespace Rendering
{
	class IResourceManager
	{
	  public:
		virtual ~IResourceManager() = default;
		ResourcePtr GetResourceByName(const std::string &i_name) const
		{
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end())
				return it->second;
			auto fallback = m_nameResMap.find("Default");
			return fallback == m_nameResMap.end() ? nullptr : fallback->second;
		}
		ResourcePtr GetResByHandle(uint64_t i_handle) const
		{
			for (auto &pair : m_nameResMap)
				if (pair.second->GetHandle() == i_handle)
					return pair.second;
			return nullptr;
		}
		void Clear() { m_nameResMap.clear(); }

	  protected:
		std::map<std::string, ResourcePtr> m_nameResMap;
	};
} // namespace Rendering
