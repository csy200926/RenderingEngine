#pragma once
#include <string>
#include <vector>
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include "Model.h"

namespace Rendering
{
	class SceneNode
	{
	public:
		glm::vec3 m_position;
		glm::vec3 m_scale;
		glm::quat m_orientation;


		SceneNode::SceneNode() : m_pParent(nullptr), m_pRenderable(nullptr)
		{

			m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
			m_orientation = glm::quat(glm::vec3(0, 0, 0));
		}


		SceneNode::~SceneNode()
		{

		}

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

		void AttachRenderable(Model *i_pModel)
		{
			m_pRenderable = i_pModel;
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

	private:

		// Do not change this directly!
		std::string name;

		// Fuck them!

		SceneNode *m_pParent;
		std::vector<SceneNode *> m_children;

		Model* m_pRenderable;

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