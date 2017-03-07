#pragma once
#include <string>
#include <memory>


namespace Rendering
{
	class IResourceManager;

	enum ResourceType
	{
		RT_Texture,
		RT_Mesh,
		RT_Shader,
		RT_Material,
	};


	class IResource
	{
	public:
		virtual ~IResource(){};

		ResourceType GetType() const
		{
			return m_type;
		}

		virtual void Load(){};


	protected:
		IResourceManager* m_pCreator;

		std::string m_name;
		std::string m_fileName;

		ResourceType m_type;
	};

	typedef std::shared_ptr<IResource> ResourcePtr;
}