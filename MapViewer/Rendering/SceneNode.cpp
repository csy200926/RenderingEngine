#include "SceneNode.h"

namespace Rendering
{


	void SceneNode::Draw()
	{
		using namespace glm;

		// Draw self
		if (m_pRenderable != nullptr)
		{
			Camera::ModelToWorld_Matrix = GetFullTransform();
			m_pRenderable->Draw();
		}

		// Draw children
		for (size_t index = 0; index < m_children.size(); index++)
		{
			m_children[index]->Draw();
		}
	}

	SceneNode* SceneNode::GetChildByName(std::string childObjName)
	{
		for (size_t index = 0; index < m_children.size(); index++)
		{
			if (m_children[index]->name == childObjName)
			{
				return m_children[index];
			}
		}

		return nullptr;
	}

	void SceneNode::RemoveChild(std::string childObjName)
	{
		for (size_t index = 0; index < m_children.size(); index++)
		{
			if (m_children[index]->name == childObjName)
			{
				m_children.erase(m_children.begin() + index);
				break;
			}
		}
	}




}