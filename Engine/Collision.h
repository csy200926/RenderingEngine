#pragma once
#include "Actor.h"
#include "NamedBitSet.h"
#include "Rendering/ISingleton.h"
namespace MyEngine
{
	class Collision : public Singleton<Collision>
	{
	  public:
		void update(const TemplateArray<ActorPtr> &i_actors, double i_delta);
		static unsigned GetCollisionID(const HashedString &i_type);
		static unsigned GetCollisionMask(const HashedString &i_type);
		static unsigned GetCollisionMask(unsigned i_id);
		unsigned GetCollisionCount() const { return m_collisionCount; }

	  private:
		unsigned m_collisionCount = 0;
	};
} // namespace MyEngine
