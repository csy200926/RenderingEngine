#pragma once
#include "Actor.h"
namespace Rendering
{
	class RenderingEngine;
}
namespace MyEngine
{
	class Scene
	{
	  public:
		virtual ~Scene();
		virtual void update(double i_delta) {}
		void InternalUpdate(double i_delta);
		void Draw(Rendering::RenderingEngine &i_engine);
		void addActor(const ActorPtr &i_actor);
		// 编辑器把物体移出场景时，历史记录仍可持有它，撤销时再放回来。
		bool RemoveActor(ActorPtr i_actor);
		ActorPtr addActor(const char *i_texture, float i_width, float i_height, bool i_is3DObj);
		ActorPtr addTriggerBox(float i_x, float i_y, float i_z, bool i_is3DObj);
		void releaseDeadActors();
		void FlushPendingActors();
		ActorPtr FindActorByName(const char *i_name) const;
		TemplateArray<ActorPtr> FindAllActorsByClass(const char *i_type) const;
		bool loadActorsFromConfig(const std::string &i_path);
		const TemplateArray<ActorPtr> &GetActors() const { return Actors; }

	  private:
		TemplateArray<ActorPtr> Actors;
		TemplateArray<ActorPtr> m_pendingActors;
		bool m_isUpdating = false;
	};
} // namespace MyEngine
