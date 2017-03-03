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

		MeshRenderer(MaterialPtr i_pMaterial, MeshPtr i_pMesh):
			 m_pMaterial(i_pMaterial),
			 m_pMesh(i_pMesh)
		{

		}

		virtual void Render() const
		{
			Camera::ModelToWorld_Matrix = GetTransform();
			m_pMesh->Draw(m_pMaterial.get());
		
		}

	private:

		MaterialPtr m_pMaterial;
		MeshPtr m_pMesh;

	};



}