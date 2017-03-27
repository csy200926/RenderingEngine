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

		virtual IResource *CreateImpl(const std::string& i_path) = 0;

		virtual ~IResourceManager(){ m_nameResMap.clear(); };


		virtual ResourcePtr GetResourceByName(const std::string& i_name)
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
			
			return ResourcePtr(nullptr);

		}



	protected:

		ResourceMap m_nameResMap;

		ResourcePtr Load(const std::string& i_name, const std::string& i_path)
		{

			ResourcePtr resPtr = Create(i_path);

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