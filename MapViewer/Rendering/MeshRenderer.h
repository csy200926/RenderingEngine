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
	private:
		std::vector<MaterialPtr> m_pMaterials;
		MeshPtr m_pMesh;

	};



}