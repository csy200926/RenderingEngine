#pragma once
#include "INodeComponent.h"
#include "Mesh.h"
#include "Material.h"
namespace Rendering
{
	class MeshRenderer : public INodeComponent
	{
	  public:
		MeshRenderer(MaterialPtr i_material, MeshPtr i_mesh) : m_pMesh(std::move(i_mesh))
		{
			m_materials.push_back(std::move(i_material));
		}
		explicit MeshRenderer(MeshPtr i_mesh) : m_pMesh(std::move(i_mesh)) {}
		void Render(RenderingEngine &i_engine) override;
		// 显式覆盖与 Mesh 自带材质分开，存档时不把默认材质误当覆盖。
		void SetMesh(MeshPtr i_mesh) { m_pMesh = std::move(i_mesh); }
		const MeshPtr &GetMesh() const { return m_pMesh; }
		const std::vector<MaterialPtr> &GetMaterials() const { return m_materials; }
		bool AddMaterial(MaterialPtr i_material)
		{
			if (m_materials.size() >= static_cast<size_t>(m_pMesh->GetSubmeshCount()))
				return false;
			m_materials.push_back(std::move(i_material));
			return true;
		}

	  private:
		MeshPtr m_pMesh;
		std::vector<MaterialPtr> m_materials;
	};
} // namespace Rendering
