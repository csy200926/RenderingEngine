#pragma once
#include "IResourceManager.h"
#include "ISingleton.h"
#include "Texture.h"
namespace Rendering
{
	class TextureManager : public IResourceManager, public Singleton<TextureManager>
	{
	  public:
		TextureManager()
		{
			m_whiteTexture = std::make_shared<Texture>(1, 1, std::vector<unsigned char>{255, 255, 255, 255});
			m_nameResMap["Default"] = m_whiteTexture;
		}
		TexturePtr GetWhiteTexture() const { return m_whiteTexture; }
		TexturePtr Load(const std::string &i_name, const std::string &i_path)
		{
			auto it = m_nameResMap.find(i_name);
			if (it != m_nameResMap.end() && it->second->GetFileName() == i_path)
				return std::static_pointer_cast<Texture>(it->second);
			auto texture = std::make_shared<Texture>(i_path);
			m_nameResMap[i_name] = texture;
			return texture;
		}
		TexturePtr GetByName(const std::string &i_name) const
		{
			return std::static_pointer_cast<Texture>(GetResourceByName(i_name));
		}

	  private:
		TexturePtr m_whiteTexture;
	};
} // namespace Rendering
