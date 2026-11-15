#pragma once
#include "Actor.h"
#include "Vector3.h"
#include "IActorController.h"
#include "Extension.h"
#include "AllocationOverride.h"

namespace MyEngine{

	inline void Actor::addController(IActorController *controller)
	{
		_pActorControllers.push(controller);
	}

	inline void Actor::setPosition(const Vector3 & position)
	{
		(*_pPosition) = position;
	}
	void Actor::setVelocity(const Vector3 & velocity)
	{
		(*_pVelocity) = velocity;
	}

	inline Vector3* Actor::getPosition() const
	{
		return _pPosition;
	}

	Vector3* Actor::getVelocity()
	{
		return _pVelocity;
	}

	inline ActorState Actor::getState() const
	{
		return m_ActorState;
	}

	inline void Actor::getsKilled()
	{
		m_ActorState = ActorState_Dead;
		texture = 46;
	}

	inline void Actor::setTag(const int tag)
	{
		m_tag = tag;
	}
	
	inline int Actor::getTag() const
	{
		return m_tag;
	}


	inline void Actor::setMass(const float i_mass)
	{
		m_mass = i_mass;
	}
	
	inline float Actor::getMass() const
	{
		return m_mass;
	}

	inline Cheesy::Sprite *Actor::getSprite()
	{
		return m_pSprite;
	
	}


	inline Vector3* Actor::getRotation()
	{
		return m_rotation;
	}
	inline void Actor::setRotation(const Vector3 & i_rotation)
	{
		(*m_rotation) = i_rotation;
	}

	Vector3* Actor::getAcceleration()
	{
		return _pAcceleration;
	}

	void Actor::setAcceleration(const Vector3 & i_Acceleration)
	{
		(*_pAcceleration) = i_Acceleration;
	}


	void Actor::setClassType(const char *i_pClassType)
	{
		m_pClassType = i_pClassType;
	}

	const char * Actor::getClassType()
	{
		return m_pClassType;
	}

	void Actor::addName(const char *i_pName)
	{

		m_pName = new HashedString(i_pName);
	}


	unsigned int Actor::getName()
	{
		return m_pName->Get();
	}
	void Actor::setLifeSpan( float time )
	{
		lifeSpan = time;
	
	}
}