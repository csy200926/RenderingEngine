#pragma once
#include <string>
#include <memory>


namespace Rendering
{
	class IResourceManager;

	enum ResourceType
	{
		RT_Texture = 0,
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

		std::string m_fileName;
		std::string m_name;

	protected:

		IResourceManager* m_pCreator;

		ResourceType m_type;
	};

	typedef std::shared_ptr<IResource> ResourcePtr;
}