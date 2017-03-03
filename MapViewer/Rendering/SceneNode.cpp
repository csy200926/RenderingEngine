#include "SceneNode.h"
#include "INodeComponent.h"
namespace Rendering
{


	void SceneNode::Draw()
	{
		using namespace glm;

		// Draw components
		for (size_t index = 0; index < m_components.size(); index++)
		{
			m_components[index]->Render();
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

	void SceneNode::InternalUpdate()
	{
		// Update components
		for (size_t index = 0; index < m_components.size(); index++)
		{
			m_components[index]->Update();
		}

		// Update children
		for (size_t index = 0; index < m_children.size(); index++)
		{
			m_children[index]->InternalUpdate();
		}
	}

	void SceneNode::AttachComponent(INodeComponent *i_component)
	{
		if (i_component->GetParentNode() == nullptr)
		{
			i_component->SetNode(this);
			m_components.push_back(i_component);
		}
	}




}