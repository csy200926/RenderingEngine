#pragma once
#include "Engine/Vector3.h"
#include "INodeComponent.h"
#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include <map>
#include <memory>
#include <vector>
#include <typeindex>
#include <string>
#include <stdexcept>
namespace Rendering
{
	inline glm::vec3 ToGLM(const MyEngine::Vector3 &i_value)
	{
		return {i_value.getX(), i_value.getY(), i_value.getZ()};
	}
	inline MyEngine::Vector3 FromGLM(const glm::vec3 &i_value)
	{
		return {i_value.x, i_value.y, i_value.z};
	}
	class RenderingEngine;
	class SceneNode
	{
	  public:
		explicit SceneNode(std::string i_name = "Node");
		virtual ~SceneNode();
		SceneNode(const SceneNode &) = delete;
		SceneNode &operator=(const SceneNode &) = delete;
		MyEngine::Vector3 m_position;
		glm::vec3 m_scale = glm::vec3(1);
		glm::quat m_orientation = glm::quat(1, 0, 0, 0);
		template <class T, class... Args> T *AddComponent(Args &&...i_args)
		{
			auto index = std::type_index(typeid(T));
			auto it = m_components.find(index);
			if (it != m_components.end())
				return static_cast<T *>(it->second.get());
			auto pComponent = std::make_unique<T>(std::forward<Args>(i_args)...);
			pComponent->SetNode(this);
			auto pResult = pComponent.get();
			m_components.emplace(index, std::move(pComponent));
			return pResult;
		}
		template <class T> T *GetComponent() const
		{
			auto it = m_components.find(std::type_index(typeid(T)));
			return it == m_components.end() ? nullptr : static_cast<T *>(it->second.get());
		}
		void AddChild(SceneNode *i_pChild);
		void RemoveChild(const std::string &i_name);
		SceneNode *GetChildByName(const std::string &i_name) const;
		SceneNode *GetParent() const { return m_pParent; }
		// 场景存档只读取节点与组件；所有权仍由节点内部管理。
		const std::vector<std::unique_ptr<SceneNode>> &GetChildren() const { return m_children; }
		const std::map<std::type_index, std::unique_ptr<INodeComponent>> &GetComponents() const { return m_components; }
		const std::string &GetNodeName() const { return m_name; }
		void SetNodeName(const std::string &i_name) { m_name = i_name; }
		glm::mat4 GetLocalTransform() const;
		glm::mat4 GetFullTransform() const;
		void Roll(float i_angle) { Rotate(glm::vec3(0, 0, 1), i_angle); }
		void Pitch(float i_angle) { Rotate(glm::vec3(1, 0, 0), i_angle); }
		void Yaw(float i_angle) { Rotate(glm::vec3(0, 1, 0), i_angle); }
		void Rotate(const glm::vec3 &i_axis, float i_angle);
		virtual void Update(double i_delta) {}
		virtual bool IsActor() const { return false; }
		void InternalUpdate(double i_delta);
		virtual void Draw(RenderingEngine &i_engine);

	  private:
		SceneNode *m_pParent = nullptr;
		std::string m_name;
		std::map<std::type_index, std::unique_ptr<INodeComponent>> m_components;
		std::vector<std::unique_ptr<SceneNode>> m_children;
		std::vector<std::unique_ptr<SceneNode>> m_pendingChildren;
		std::vector<std::string> m_pendingRemovals;
		bool m_isUpdating = false;
		void FlushPendingChildren();
	};
} // namespace Rendering
