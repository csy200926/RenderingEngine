#pragma once
#include "INodeComponent.h"

#include <memory>
#include "Material.h"
#include "Mesh.h"
#include "Camera.h"
#include <vector>
namespace Rendering
{


	class MeshRenderer : public INodeComponent
	{
	public:
		MeshRenderer(){}

		MeshRenderer(const MaterialPtr &i_pMaterial, const MeshPtr &i_pMesh)
		{
			m_pMaterials.push_back(i_pMaterial);
			m_pMesh = i_pMesh;
		}

		virtual ~MeshRenderer()
		{

		}

		virtual void Render()
		{
			m_pMesh->Draw_Pre();
			Camera::ModelToWorld_Matrix = GetTransform();
			for (int i = 0; i < m_pMaterials.size(); i++)
			{
				m_pMaterials[i]->Activate();
				m_pMesh->Draw(i);
			}
			m_pMesh->Draw_Post();
		} 

		bool AddMaterial(MaterialPtr i_pMaterial)
		{
			if (m_pMesh->GetSubmeshCount() >= m_pMaterials.size() + 1)
			{
				m_pMaterials.push_back(i_pMaterial);
				return true;
			}
			return false;
		}
		void Serialize(LuaPlus::LuaObject &luaObject)
		{
			using namespace LuaPlus;
			LuaObject materials_lua = luaObject.CreateTable("Materials");

			for (int i = 0; i < m_pMaterials.size(); i++)
			{
				materials_lua.SetString(i, m_pMaterials[i]->m_name.c_str());
			}

			luaObject.SetString("Mesh", m_pMesh->m_name.c_str());
		}

		void Deserialize(LuaPlus::LuaObject &luaObject)
		{
			int debug = 0;
		}
	private:
		std::vector<MaterialPtr> m_pMaterials;
		MeshPtr m_pMesh;

	};



}