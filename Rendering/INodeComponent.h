#pragma once
#include <glm.hpp>
namespace Rendering
{
	class SceneNode;
	class RenderingEngine;
	class INodeComponent
	{
	  public:
		virtual ~INodeComponent() = default;
		virtual void Update(double i_delta) {}
		virtual void Render(RenderingEngine &i_engine) {}
		glm::mat4 GetTransform() const;
		SceneNode *GetParentNode() const { return m_pNode; }
		virtual void SetNode(SceneNode *i_pNode) { m_pNode = i_pNode; }

	  protected:
		SceneNode *m_pNode = nullptr;
	};
} // namespace Rendering
