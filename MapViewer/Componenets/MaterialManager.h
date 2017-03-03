#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Material.h"

namespace Rendering
{
	class MaterialManager : public IResourceManager , public Singleton<MaterialManager>
	{
	public:
		virtual IResource *CreateImpl(std::string& i_path)
		{
			Material *pMaterial;
			pMaterial = new Material(i_path);
			return pMaterial;
		}
		




	};











}
