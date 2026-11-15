#pragma once
#include "Rendering/SceneNode.h"
#include "IIsA.h"
#include "TemplateArray.h"
#include "MySharedPointer.h"
#include "IActorController.h"
#include "ICollisionCallBack.h"
namespace Rendering
{
	class Texture;
}
namespace MyEngine
{
	class Scene;
	enum ActorState
	{
		ActorState_Dead = 0,
		ActorState_Alive,
		ActorState_Frosen,
		ActorState_AvatarState
	};
	struct Center
	{
		float x = 0, y = 0, z = 0;
	};
	struct AABB
	{
		Center center;
		float HalfX = .5f, HalfY = .5f, HalfZ = .5f;
	};
	class Actor : public Rendering::SceneNode, public IIsA
	{
	  public:
		explicit Actor(const std::string &i_name = "Actor");
		Actor(float i_bx, float i_by, float i_bz, bool i_is3DObj);
		Actor(const char *i_textureName, float i_width, float i_height, bool i_is3DObj);
		virtual ~Actor();
		static void *operator new(size_t i_size);
		static void operator delete(void *i_pPtr) noexcept;
		static void releaseMemoryPool();
		bool IsActor() const override { return true; }
		void Update(double i_delta) override;
		void projectMove(double i_delta);
		void finalizeMove();
		void addController(IActorController *i_pController);
		void removeController(IActorController *i_pController);
		void setPosition(const Vector3 &i_position) { m_position = i_position; }
		Vector3 *getPosition() { return &m_position; }
		const Vector3 *getPosition() const { return &m_position; }
		void setVelocity(const Vector3 &i_value) { m_velocity = i_value; }
		Vector3 *getVelocity() { return &m_velocity; }
		const Vector3 *getVelocity() const { return &m_velocity; }
		void setAcceleration(const Vector3 &i_value) { m_acceleration = i_value; }
		Vector3 *getAcceleration() { return &m_acceleration; }
		void setRotation(const Vector3 &i_value);
		Vector3 getRotation() const;
		ActorState getState() const { return m_ActorState; }
		void getsKilled() { m_ActorState = ActorState_Dead; }
		void setTag(int i_tag) { m_tag = i_tag; }
		int getTag() const { return m_tag; }
		void setMass(float i_mass) { m_mass = i_mass > 0 ? i_mass : 0; }
		float getMass() const { return m_mass; }
		void setClassType(const char *i_type) { m_classType = i_type; }
		const char *getClassType() const { return m_classType.c_str(); }
		void addName(const char *i_name) { SetNodeName(i_name); }
		unsigned getName() const { return HashedString::Hash(GetNodeName().c_str()); }
		void setLifeSpan(float i_time) { lifeSpan = i_time; }
		bool is3DObj = true;
		bool isTriggerBox = false;
		bool m_isStatic = false;
		bool m_collisionEnabled = false;
		float m_collisionTime = 0;
		ICollisionCallBack *m_collisionCallBack = nullptr; // Borrowed; controller/scene owns callback.
		unsigned m_CollisionLogicalResponseIDMask = 0, m_CollisionID = 0;
		AABB boundingBox;
		Vector3 m_previousPosition;
		Vector3 m_ProjectedPosition;
		HashedString materialName, meshName;

	  private:
		friend class Scene;
		Scene *m_pScene = nullptr;
		TemplateArray<IActorController *> m_actorControllers;
		Vector3 m_velocity, m_acceleration;
		ActorState m_ActorState = ActorState_Alive;
		std::string m_classType = "Actor";
		float lifeSpan = -1, m_mass = 1;
		int m_tag = 0;
	};
	using ActorPtr = MySharedPointer<Actor>;
} // namespace MyEngine
