#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Texture.h"

namespace Rendering
{
	class TextureManager : public IResourceManager, public Singleton < TextureManager >
	{
	public:
		virtual IResource *CreateImpl(std::string& i_path)
		{
			Texture *pTexture;
			pTexture = new Texture();
			pTexture->LoadFromPath(i_path.c_str());

			return pTexture;
		}





	};

}
