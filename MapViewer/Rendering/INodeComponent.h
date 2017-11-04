#pragma once


#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <glm.hpp>
#include <gtx/quaternion.hpp>

#include "SceneNode.h"
#include "../ISerializable.h"

namespace Rendering
{


	class INodeComponent : public ISerializable
	{

	public:
		INodeComponent() :m_pNode(nullptr){};
		virtual ~INodeComponent()		{
		
		}

		virtual void Update(){}
		virtual void Render(){}

		// TODO: pass class 'Transform' directly
		glm::mat4x4 GetTransform()
		{ 
			return m_pNode->GetFullTransform();
		}

		SceneNode *GetParentNode() 
		{ 
			return m_pNode;
		}

		virtual void SetNode(SceneNode *i_pNode)
		{
			m_pNode = i_pNode;
		}

		virtual void Serialize(LuaPlus::LuaObject luaObject){};
		virtual void Deserialize(LuaPlus::LuaObject luaObject){};

		SceneNode *m_pNode;
		//virtual void AddToEngine(CoreEngine* engine) const { }
	private:
		

	};
}
