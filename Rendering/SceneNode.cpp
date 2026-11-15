#include "SceneNode.h"
#include <gtc/matrix_transform.hpp>
#include <algorithm>
namespace Rendering
{
	SceneNode::SceneNode(std::string i_name) : m_name(std::move(i_name))
	{
	}
	SceneNode::~SceneNode() = default;
	glm::mat4 INodeComponent::GetTransform() const
	{
		return m_pNode->GetFullTransform();
	}
	void SceneNode::AddChild(SceneNode *i_pChild)
	{
		if (!i_pChild || i_pChild->IsActor() || i_pChild->m_pParent)
			throw std::logic_error("AddChild owns plain SceneNodes only; Scene owns Actors");
		for (auto pNode = this; pNode; pNode = pNode->m_pParent)
			if (pNode == i_pChild)
				throw std::logic_error("SceneNode parent cycle");
		i_pChild->m_pParent = this;
		if (m_isUpdating)
			m_pendingChildren.emplace_back(i_pChild);
		else
			m_children.emplace_back(i_pChild);
	}
	void SceneNode::RemoveChild(const std::string &i_name)
	{
		if (m_isUpdating)
		{
			m_pendingRemovals.push_back(i_name);
			return;
		}
		m_children.erase(std::remove_if(m_children.begin(), m_children.end(),
										[&](const auto &pChild) { return pChild->m_name == i_name; }),
						 m_children.end());
	}
	SceneNode *SceneNode::GetChildByName(const std::string &i_name) const
	{
		for (const auto &pChild : m_children)
			if (pChild->m_name == i_name)
				return pChild.get();
		return nullptr;
	}
	glm::mat4 SceneNode::GetLocalTransform() const
	{
		return glm::translate(glm::mat4(1), ToGLM(m_position)) * glm::mat4_cast(m_orientation) *
			   glm::scale(glm::mat4(1), m_scale);
	}
	glm::mat4 SceneNode::GetFullTransform() const
	{
		return m_pParent ? m_pParent->GetFullTransform() * GetLocalTransform() : GetLocalTransform();
	}
	void SceneNode::Rotate(const glm::vec3 &i_axis, float i_angle)
	{
		m_orientation = glm::normalize(m_orientation * glm::angleAxis(glm::radians(i_angle), glm::normalize(i_axis)));
	}
	void SceneNode::InternalUpdate(double i_delta)
	{
		m_isUpdating = true;
		Update(i_delta);
		std::vector<INodeComponent *> components;
		for (auto &pair : m_components)
			components.push_back(pair.second.get());
		for (auto pComponent : components)
			pComponent->Update(i_delta);
		for (auto &pChild : m_children)
			pChild->InternalUpdate(i_delta);
		m_isUpdating = false;
		FlushPendingChildren();
	}
	void SceneNode::FlushPendingChildren()
	{
		for (auto &pChild : m_pendingChildren)
			m_children.push_back(std::move(pChild));
		m_pendingChildren.clear();
		for (auto &name : m_pendingRemovals)
			RemoveChild(name);
		m_pendingRemovals.clear();
	}
	void SceneNode::Draw(RenderingEngine &i_engine)
	{
		m_isUpdating = true;
		std::vector<INodeComponent *> components;
		for (auto &pair : m_components)
			components.push_back(pair.second.get());
		for (auto pComponent : components)
			pComponent->Render(i_engine);
		for (auto &pChild : m_children)
			pChild->Draw(i_engine);
		m_isUpdating = false;
		FlushPendingChildren();
	}
} // namespace Rendering
