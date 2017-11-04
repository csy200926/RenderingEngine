#include "SceneNode.h"
#include "INodeComponent.h"
namespace Rendering
{


	void SceneNode::Draw()
	{
		using namespace glm;

		// Draw components
		TypeComponentMap::iterator it = m_components.begin();
		for (; it != m_components.end(); it++)
		{
			it->second->Render();
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
			if (m_children[index]->m_name == childObjName)
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
			if (m_children[index]->m_name == childObjName)
			{
				m_children.erase(m_children.begin() + index);
				break;
			}
		}
	}

	void SceneNode::InternalUpdate()
	{
		//// Update components
		TypeComponentMap::iterator it = m_components.begin();
		for (; it != m_components.end(); it++)
		{
			it->second->Update();
		}

		// Update children
		for (size_t index = 0; index < m_children.size(); index++)
		{
			m_children[index]->InternalUpdate();
		}
	}

	SceneNode::~SceneNode()
	{
		TypeComponentMap::iterator it = m_components.begin();
		for (; it != m_components.end(); it++)
		{
			delete it->second;
		}
		for (int i = 0; i < m_children.size(); i++)
		{
			if (m_children[i] != nullptr)
				delete m_children[i];
		}
	}

	void SceneNode::Serialize(LuaPlus::LuaObject luaObject)
	{
		using namespace LuaPlus;
		//LuaObject components = luaObject.CreateTable("Components");
		//TypeComponentMap::iterator it = m_components.begin();
		//for (; it != m_components.end(); it++)
		//{
		//	LuaObject component = components.CreateTable(components.);
		//	it->second.Serialize();
		//}


	}

	void SceneNode::Deserialize(LuaPlus::LuaObject luaObject)
	{

	}





}