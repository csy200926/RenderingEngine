#pragma once
#include <string>
#include <map>
#include <memory>

#include "IResources.h"
#include "../ISerializable.h"

namespace Rendering
{

	typedef std::map<std::string, ResourcePtr> ResourceMap;
	class IResourceManager : public ISerializable
	{

	public:

		virtual IResource *CreateImpl(const std::string& i_path) = 0;

		virtual ~IResourceManager(){ m_nameResMap.clear(); };


		virtual ResourcePtr GetResourceByName(const std::string& i_name)
		{
			ResourcePtr resPtr;
			ResourceMap::iterator it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end())
			{
				resPtr = it->second;
			}
			else
				return m_nameResMap["Default"];
			return resPtr;
		}


		virtual ResourcePtr GetResByHandle(int temp)
		{
			return ResourcePtr(nullptr);
		}

		virtual void Serialize(LuaPlus::LuaObject &luaObject)
		{
			ResourceMap::iterator it = m_nameResMap.begin();
			for (; it != m_nameResMap.end(); it++)
			{
				LuaObject resFile_lua = luaObject.CreateTable("ResourcesFile");

			}

		}

		virtual void Deserialize(LuaPlus::LuaObject &luaObject)
		{

		}

	protected:

		ResourceMap m_nameResMap;

		ResourcePtr Load(const std::string& i_name, const std::string& i_path)
		{

			ResourcePtr resPtr = Create(i_path);
			resPtr->m_name = i_name;
			resPtr->m_fileName = i_path;

			m_nameResMap.insert(make_pair(i_name, resPtr));

			return resPtr;

		}
	private:
		virtual ResourcePtr Create(const std::string& i_path)
		{
			ResourcePtr resPtr;

			resPtr = ResourcePtr(CreateImpl(i_path));

			return resPtr;
		}
	};
}