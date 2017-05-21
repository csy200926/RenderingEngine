#pragma once
#include "Lighting.h"
#include "SceneNode.h"
#include "ISingleton.h"
#include <vector>
#include "Material.h"
#include <glm.hpp>
#include "SkyBox.h"
#include "TextureCube.h"

namespace Rendering
{

#define FRONT "Images/skyBox/stormydays_ft.tga"
#define BACK "Images/skyBox/stormydays_bk.tga"
#define TOP "Images/skyBox/stormydays_up.tga"
#define BOTTOM "Images/skyBox/stormydays_dn.tga"
#define LEFT "Images/skyBox/stormydays_lf.tga"
#define RIGHT "Images/skyBox/stormydays_rt.tga"

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

		SkyBox m_skyBox;
		TextureCube m_cubeTex;

		GLuint framebuffer;
		GLuint renderTexture;
		GLuint depthrenderBuffer;
		GLuint quad_vertexbuffer;

		// AA use
		GLuint multiTex;
		GLuint framebuffer_AA;

		// PBR
		MaterialPtr m_PBRcon;
		GLuint m_envCubemap;
	};
}