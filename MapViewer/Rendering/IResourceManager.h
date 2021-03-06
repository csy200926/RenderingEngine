#pragma once
#include <string>
#include <map>
#include <memory>

#include "IResources.h"
#include "../ISerializable.h"

#include <fstream>

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
			using namespace LuaPlus;
			ResourceMap::iterator it = m_nameResMap.begin();
			for (; it != m_nameResMap.end(); it++)
			{
				LuaObject resFile_lua = luaObject.CreateTable(it->second->m_name.c_str());
				resFile_lua.SetString("Path", it->second->m_fileName.c_str());
			}

		}

		virtual void Deserialize(LuaPlus::LuaObject &luaObject)
		{

		}

	protected:

		ResourceMap m_nameResMap;

		ResourcePtr Load(const std::string& i_name, const std::string& i_path)
		{
			if (m_nameResMap.find(i_name) != m_nameResMap.end())
				return nullptr;

			if (IsFileExist(i_path.c_str()) == false)
				return nullptr;

			ResourcePtr resPtr = Create(i_path);
			

			resPtr->m_name = i_name;
			resPtr->m_fileName = i_path;

			m_nameResMap.insert(make_pair(i_name, resPtr));

			return resPtr;

		}
	private:


		bool IsFileExist(std::string fileName)
		{
			std::ifstream infile(fileName.c_str());
			return infile.good();
		}
		virtual ResourcePtr Create(const std::string& i_path)
		{
			ResourcePtr resPtr;

			resPtr = ResourcePtr(CreateImpl(i_path));

			return resPtr;
		}
	};
}