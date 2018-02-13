#pragma once
#include <string>
#include <vector>
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include <map>
#include <typeindex>

#include "../ISerializable.h"

namespace Rendering
{
	class INodeComponent;
	typedef std::map<std::type_index, INodeComponent *> TypeComponentMap;

	class SceneNode : public ISerializable
	{


	public:
		glm::vec3 m_position;
		glm::vec3 m_scale;
		glm::quat m_orientation;

		SceneNode(){}

		SceneNode::SceneNode(std::string i_nodeName) : m_pParent(nullptr), m_name(i_nodeName)
		{

			m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
			m_orientation = glm::quat(glm::vec3(0, 0, 0));
		}

		
		~SceneNode();

		// Rotate around local Z
		void Roll(const float angle)
		{
			Rotate(glm::vec3(0,0,1), angle);
		}

		// Rotate around local X
		void Pitch(const float angle)
		{
			Rotate(glm::vec3(1,0,0), angle);
		}

		// Rotate around local Y
		void Yaw(const float angle)
		{
			Rotate(glm::vec3(0,1,0), angle);
		}

		void Rotate(const glm::vec3 &axis, const float angle)
		{
			using namespace glm;
			quat q;
			q = angleAxis(degrees(angle), axis);
			m_orientation = m_orientation * q;

		}

		template<typename T, typename ...Args>
		T* AddComponent(Args&&... args)
		{
			using namespace std;

			T* pComponent = new T(args...);
			std::type_index index(typeid(T));
			//printf(index.name());
			INodeComponent *pComponentBase = static_cast<INodeComponent*>(pComponent);
			pComponentBase->SetNode(this);

			if (m_components.find(index) == m_components.end())
				m_components.insert(make_pair(index, pComponentBase));
			else
				printf("SceneNode-AddComponent:only one instance of each type can be added.");

			return pComponent;
		}
		template<typename T>
		T* GetComponent()
		{
			using namespace std;

			std::type_index index(typeid(T));
			TypeComponentMap::iterator it_result = m_components.find(index);

			if (it_result != m_components.end())
				return static_cast<T*>(it_result->second);
			else
				return nullptr;
		}

		void AddChild(SceneNode *pChildObj)
		{
			m_children.push_back(pChildObj);
			pChildObj->m_pParent = this;
		}

		void RemoveChild(std::string childObjName);
		SceneNode* GetChildByName(std::string childObjName);

		glm::mat4x4 GetFullTransform()
		{
			if (m_pParent != nullptr)
				return m_pParent->GetFullTransform() * GetLocalTransform();
			return GetLocalTransform();
		}

		// TODO: transform cache
		virtual void Draw();

		virtual void Update(){};
		void InternalUpdate();

		SceneNode *m_pParent;

		virtual void Serialize(LuaPlus::LuaObject &luaObject);
		virtual void Deserialize(LuaPlus::LuaObject &luaObject);
	private:

		std::string m_name;

		std::vector<SceneNode *> m_children;

		TypeComponentMap m_components;


		//Stop thinking those fucking shit!
		glm::mat4x4 GetLocalTransform()
		{
			using namespace glm;

			mat4 scale = glm::scale(mat4(1.0f), m_scale);
			mat4 rotation = glm::mat4_cast(m_orientation);
			mat4 translation = glm::translate(m_position);

			return translation * rotation * scale;
		};
	};



}
