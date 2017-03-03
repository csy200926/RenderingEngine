#pragma once
#include <string>
#include <map>
#include <memory>

#include "IResources.h"
namespace Rendering
{

	typedef std::map<std::string, ResourcePtr> ResourceMap;
	class IResourceManager
	{

	public:

		virtual IResource *CreateImpl(std::string& i_path) = 0;


		virtual void Load(std::string i_name,std::string& i_path)
		{

			ResourcePtr resPtr = Create(i_path);
			m_nameResMap.insert(make_pair(i_name, resPtr));
			
		}


		virtual ResourcePtr GetResByName(std::string& i_name)
		{
			ResourcePtr resPtr;
			ResourceMap::iterator it = m_nameResMap.find(i_name);
			for (; it != m_nameResMap.end(); it++)
			{
				resPtr = it->second;
			}
			return resPtr;
		}


		virtual ResourcePtr GetResByHandle(int temp)
		{
			


		}


		virtual ResourcePtr Create(std::string& i_path)
		{
			ResourcePtr resPtr;

			resPtr = ResourcePtr(CreateImpl(i_path));

			return resPtr;
		}

	protected:
		ResourceMap m_nameResMap;

	};
}