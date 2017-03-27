#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Material.h"

namespace Rendering
{
	class MaterialManager : public IResourceManager , public Singleton<MaterialManager>
	{
	public:
		virtual IResource *CreateImpl(const std::string& i_path)
		{
			Material *pMaterial;
			pMaterial = new Material(i_path);
			return pMaterial;
		}
		

		MaterialPtr Load(const std::string& i_name, const std::string& i_path)
		{
			return std::static_pointer_cast<Material>(IResourceManager::Load(i_name,i_path));
		}

		MaterialPtr GetByName(const std::string & i_name)
		{
			return std::static_pointer_cast<Material>(IResourceManager::GetResourceByName(i_name));
		}

	};











}
