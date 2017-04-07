#pragma once
#include "Lighting.h"
#include "SceneNode.h"
#include "ISingleton.h"
#include <vector>
#include "Material.h"
#include <glm.hpp>
namespace Rendering
{
	class RenderingEngine : public Singleton<RenderingEngine>
	{

	public:

		void AddLight(LightBase *i_pLight)
		{ 
			if (i_pLight->GetType() == POINT)
			{
				m_pPointLights.push_back(i_pLight);
			}
			else if (i_pLight->GetType() == DIRECTIONAL)
			{
				m_pDirectionalLight = static_cast<DirectionalLight*>( i_pLight );
			}
		}
		void RemoveLight(LightBase *i_pLight)
		{
			if (i_pLight->GetType() == POINT)
			{
				for (size_t i = 0; i < m_pPointLights.size(); i++)
				{
					if (m_pPointLights[i] == i_pLight)
					{
						m_pPointLights.erase(m_pPointLights.begin() + i);
						return;
					}
				}
			}
			else if (i_pLight->GetType() == DIRECTIONAL)
			{
				m_pDirectionalLight = nullptr;
			}
		}
		void SetRootNode(SceneNode* i_pRoot)
		{
			m_pRootNode = i_pRoot;
		}
		virtual void Render();
		virtual void OnRenderImage(TexturePtr scr, TexturePtr tar, MaterialPtr material);

		void Initilize();
		void ShutDown();
	private:

		void UpdatePointLightUniform(Material *i_pMaterial);
		void UpdateDirctLightUniform(Material *i_pMaterial);

		MaterialPtr m_defaultMat;
		MaterialPtr m_screenMat;

		SceneNode *m_pRootNode;
		std::vector<LightBase*> m_pPointLights;
		DirectionalLight* m_pDirectionalLight;


		GLuint framebuffer;
		GLuint renderTexture;
		GLuint depthrenderBuffer;
		GLuint quad_vertexbuffer;
	};
}