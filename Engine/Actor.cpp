#include "Actor.h"
#include "MemoryPool.h"
#include "Rendering/SpriteRenderer.h"
#include "Rendering/TextureManager.h"
#include <algorithm>
#include <map>
#include <memory>
#include <cassert>
namespace MyEngine
{
	static std::map<size_t, std::unique_ptr<MemoryPool>> s_actorPools;
	void *Actor::operator new(size_t i_size)
	{
		auto &pPool = s_actorPools[i_size];
		if (!pPool)
			pPool.reset(MemoryPool::create(i_size, 64));
		return pPool->alloc();
	}
	void Actor::operator delete(void *i_pPtr) noexcept
	{
		for (auto &pair : s_actorPools)
			if (pair.second->Contains(i_pPtr))
			{
				pair.second->free(i_pPtr);
				return;
			}
		::operator delete(i_pPtr);
	}
	void Actor::releaseMemoryPool()
	{
		for (const auto &pair : s_actorPools)
			if (pair.second->GetLiveCount())
				throw std::logic_error("Actors still live while releasing pool");
		s_actorPools.clear();
	}
	Actor::Actor(const std::string &i_name) : SceneNode(i_name), IIsA("Actor")
	{
	}
	Actor::Actor(float i_bx, float i_by, float i_bz, bool i_is3DObj) : Actor("Trigger")
	{
		is3DObj = i_is3DObj;
		isTriggerBox = true;
		m_isStatic = true;
		m_collisionEnabled = true;
		boundingBox.HalfX = i_bx;
		boundingBox.HalfY = i_by;
		boundingBox.HalfZ = i_bz;
	}
	Actor::Actor(const char *i_textureName, float i_width, float i_height, bool i_is3DObj) : Actor("Actor")
	{
		is3DObj = i_is3DObj;
		boundingBox.HalfX = i_width * .5f;
		boundingBox.HalfY = i_height * .5f;
		boundingBox.HalfZ = i_is3DObj ? i_width * .5f : 1;
		if (!is3DObj)
			AddComponent<Rendering::SpriteRenderer>(
				Rendering::TextureManager::GetInstance()->Load(i_textureName, i_textureName),
				glm::vec2(i_width, i_height));
	}
	Actor::~Actor() = default;
	void Actor::addController(IActorController *i_pController)
	{
		if (!i_pController)
			return;
		if (std::find(m_actorControllers.begin(), m_actorControllers.end(), i_pController) == m_actorControllers.end())
			m_actorControllers.push(i_pController);
	}
	void Actor::removeController(IActorController *i_pController)
	{
		m_actorControllers.remove(i_pController);
	}
	void Actor::setRotation(const Vector3 &i_value)
	{
		m_orientation = glm::quat(glm::radians(Rendering::ToGLM(i_value)));
	}
	Vector3 Actor::getRotation() const
	{
		return Rendering::FromGLM(glm::degrees(glm::eulerAngles(m_orientation)));
	}
	void Actor::Update(double i_delta)
	{
		if (m_ActorState != ActorState_Alive)
			return;
		auto controllers = m_actorControllers;
		bool isDone = false;
		for (auto pController : controllers)
		{
			if (m_ActorState == ActorState_Dead)
				break;
			pController->UpdateActor(*this, isDone, i_delta);
		}
		if (lifeSpan >= 0)
		{
			lifeSpan -= static_cast<float>(i_delta);
			if (lifeSpan <= 0)
				getsKilled();
		}
	}
	void Actor::projectMove(double i_delta)
	{
		m_previousPosition = m_position;
		if (!m_isStatic && m_ActorState == ActorState_Alive)
		{
			m_velocity += m_acceleration * static_cast<float>(i_delta);
			m_ProjectedPosition = m_position + m_velocity * static_cast<float>(i_delta);
		}
		else
			m_ProjectedPosition = m_position;
	}
	void Actor::finalizeMove()
	{
		if (m_ActorState != ActorState_Dead)
			m_position = m_ProjectedPosition;
	}
} // namespace MyEngine
