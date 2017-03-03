#pragma once
#include "SceneNode.h"

#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <glm.hpp>
#include <gtx/quaternion.hpp>

namespace Rendering
{
	class Material;

	class INodeComponent
	{

	public:
		INodeComponent();
		virtual ~INodeComponent();

		virtual void Update(){}
		virtual void Render(const Material* pMaterial) const{}

		// TODO: pass class 'Transform' directly
		glm::mat4x4 GetTransform(){ m_pNode->GetFullTransform(); }

		virtual void SetNode(SceneNode *i_pNode){ m_pNode = i_pNode; }
	private:
		SceneNode *m_pNode;


	};
}
