#pragma once
#include "INodeComponent.h"

#include <memory>
#include "Material.h"
#include "Mesh.h"
#include "Camera.h"

namespace Rendering
{


	class MeshRenderer : public INodeComponent
	{
	public:


		MeshRenderer(const MaterialPtr &i_pMaterial, const MeshPtr &i_pMesh)
		{
			m_pMaterial = i_pMaterial;
			m_pMesh = i_pMesh;
		}

		virtual ~MeshRenderer()
		{

		}

		virtual void Render()
		{

			// I will just let it crash if not initialized...
			Camera::ModelToWorld_Matrix = GetTransform();
			m_pMaterial->Activate();
			m_pMesh->Draw();
		
		} 

	private:

		MaterialPtr m_pMaterial;
		MeshPtr m_pMesh;

	};



}