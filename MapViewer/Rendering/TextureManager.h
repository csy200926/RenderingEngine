#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Texture.h"

namespace Rendering
{
	class TextureManager : public IResourceManager, public Singleton < TextureManager >
	{
	public:
		virtual IResource *CreateImpl(const std::string& i_path)
		{
			Texture *pTexture;
			pTexture = new Texture();
			pTexture->LoadFromPath(i_path.c_str());

			return pTexture;
		}

		TexturePtr Load(const std::string& i_name, const std::string& i_path)
		{
			return std::static_pointer_cast<Texture>(IResourceManager::Load(i_name,i_path));
		}



	};

}
