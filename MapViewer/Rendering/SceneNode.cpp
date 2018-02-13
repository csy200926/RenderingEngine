#include "SceneNode.h"
#include "INodeComponent.h"
#include "MeshRenderer.h"
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

	void SceneNode::Serialize(LuaPlus::LuaObject &luaObject)
	{
		using namespace LuaPlus;
		using namespace std;

		// Serialize transform
		LuaObject position_lua = luaObject.CreateTable("Position");
		LuaObject scale_lua = luaObject.CreateTable("Scale");
		LuaObject orientation_lua = luaObject.CreateTable("Orientation");
		ISerializable::SerilizeVec3(position_lua, m_position);
		ISerializable::SerilizeVec3(scale_lua, m_scale);
		ISerializable::SerilizeQuat(orientation_lua, m_orientation);

		// Components
		if (m_components.size() > 0)
		{
			LuaObject components = luaObject.CreateTable("Components");
			TypeComponentMap::iterator it = m_components.begin();
			for (; it != m_components.end(); it++)
			{
				string name = SerializableFactory::GetStringByTypeIndex(it->first);
				LuaObject component_lua = components.CreateTable(name.c_str());
				it->second->Serialize(component_lua);
			}
		}

		// Children nodes
		if (m_children.size() > 0)
		{
			LuaObject children_lua = luaObject.CreateTable("Children");
			for (int i = 0; i < m_children.size(); i++)
			{
				m_children[i]->Serialize(children_lua.CreateTable(m_children[i]->m_name.c_str()));
			}
		}

	}

	void SceneNode::Deserialize(LuaPlus::LuaObject &luaObject)
	{
		using namespace LuaPlus;
		using namespace std;

		// Deserialize transform
		LuaObject componentsObj = luaObject["Components"];
		for (LuaTableIterator it(componentsObj); it; it.Next())
		{
			
			const char* componentName = it.GetKey().GetString();
			INodeComponent* pComponent = static_cast<INodeComponent*>(CREATE_CLASS(componentName));
			
			LuaObject &obj = it.GetValue();
			pComponent->Deserialize(obj);

			std::type_index index(typeid(pComponent));

			std::type_index index2(typeid(Rendering::MeshRenderer));
			int i = 0;

		}




	}





}