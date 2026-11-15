#include "Collision.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace MyEngine
{
	static NamedBitSet<unsigned> s_collisionIDs;
	unsigned Collision::GetCollisionID(const HashedString &i_type)
	{
		return s_collisionIDs.GetBitIndex(i_type);
	}
	unsigned Collision::GetCollisionMask(const HashedString &i_type)
	{
		return s_collisionIDs.GetBitMask(i_type);
	}
	unsigned Collision::GetCollisionMask(unsigned i_id)
	{
		return i_id < 32 ? (1u << i_id) : 0;
	}
	struct WorldBox
	{
		glm::vec3 m_center, m_extents;
		std::array<glm::vec3, 3> m_axes;
	};
	static WorldBox GetBox(const Actor &i_actor)
	{
		glm::mat4 transform = i_actor.GetFullTransform();
		WorldBox result;
		auto &box = i_actor.boundingBox;
		result.m_center = glm::vec3(transform * glm::vec4(box.center.x, box.center.y, box.center.z, 1));
		glm::vec3 half(box.HalfX, box.HalfY, box.HalfZ);
		for (int i = 0; i < 3; ++i)
		{
			auto axis = glm::vec3(transform[i]);
			float length = glm::length(axis);
			result.m_axes[i] = length > 1e-6f ? axis / length : glm::vec3(i == 0, i == 1, i == 2);
			result.m_extents[i] = std::abs(half[i] * length);
		}
		return result;
	}
	// Original relative-motion interval design generalized to all 15 OBB SAT axes and actual dt.
	// Rotations remain constant during a step; sheared transforms are not supported.
	static bool CheckOOBBIntersection(const WorldBox &i_a, const WorldBox &i_b, const glm::vec3 &i_relativeMove,
									  float &o_time, glm::vec3 &o_normal)
	{
		std::array<glm::vec3, 15> axes;
		for (int i = 0; i < 3; ++i)
		{
			axes[i] = i_a.m_axes[i];
			axes[i + 3] = i_b.m_axes[i];
		}
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				axes[6 + i * 3 + j] = glm::cross(i_a.m_axes[i], i_b.m_axes[j]);
		float enter = 0, exit = 1, minOverlap = 1e30f;
		glm::vec3 startNormal(0, 1, 0), enterNormal(0, 1, 0);
		glm::vec3 offset = i_b.m_center - i_a.m_center;
		for (auto axis : axes)
		{
			float length = glm::length(axis);
			if (length < 1e-5f)
				continue;
			axis /= length;
			float radius = 0;
			for (int i = 0; i < 3; ++i)
				radius += i_a.m_extents[i] * std::abs(glm::dot(axis, i_a.m_axes[i])) +
						  i_b.m_extents[i] * std::abs(glm::dot(axis, i_b.m_axes[i]));
			float distance = glm::dot(offset, axis), movement = glm::dot(i_relativeMove, axis);
			float overlap = radius - std::abs(distance);
			if (overlap < minOverlap)
			{
				minOverlap = overlap;
				startNormal = distance >= 0 ? -axis : axis;
			}
			if (std::abs(movement) < 1e-7f)
			{
				if (std::abs(distance) > radius)
					return false;
				continue;
			}
			float first = (distance - radius) / movement, last = (distance + radius) / movement;
			if (first > last)
				std::swap(first, last);
			if (first > enter)
			{
				enter = first;
				enterNormal = movement > 0 ? -axis : axis;
			}
			exit = std::min(exit, last);
			if (enter > exit)
				return false;
		}
		if (exit < 0 || enter > 1)
			return false;
		o_time = std::max(0.f, enter);
		o_normal = o_time > 0 ? enterNormal : startNormal;
		return true;
	}
	void Collision::update(const TemplateArray<ActorPtr> &i_actors, double i_delta)
	{
		for (int i = 0; i < i_actors.len; ++i)
		{
			Actor &a = *i_actors[i];
			if (!a.m_collisionEnabled || a.getState() != ActorState_Alive)
				continue;
			for (int j = i + 1; j < i_actors.len && a.getState() != ActorState_Dead; ++j)
			{
				Actor &b = *i_actors[j];
				if (!b.m_collisionEnabled || b.getState() != ActorState_Alive || (a.m_isStatic && b.m_isStatic))
					continue;
				if (!(a.m_CollisionLogicalResponseIDMask & GetCollisionMask(b.m_CollisionID)) &&
					!(b.m_CollisionLogicalResponseIDMask & GetCollisionMask(a.m_CollisionID)))
					continue;
				glm::vec3 moveA = Rendering::ToGLM(a.m_ProjectedPosition - a.m_previousPosition),
						  moveB = Rendering::ToGLM(b.m_ProjectedPosition - b.m_previousPosition);
				float time = 0;
				glm::vec3 normal;
				if (!CheckOOBBIntersection(GetBox(a), GetBox(b), moveA - moveB, time, normal))
					continue;
				++m_collisionCount;
				if (!a.isTriggerBox && !b.isTriggerBox)
				{
					glm::vec3 va = Rendering::ToGLM(*a.getVelocity()), vb = Rendering::ToGLM(*b.getVelocity());
					float inverseA = (!a.m_isStatic && a.getMass() > 0) ? 1 / a.getMass() : 0;
					float inverseB = (!b.m_isStatic && b.getMass() > 0) ? 1 / b.getMass() : 0;
					float closing = glm::dot(va - vb, normal), total = inverseA + inverseB;
					if (closing < 0 && total > 0)
					{
						float impulse = -2 * closing / total;
						va += normal * impulse * inverseA;
						vb -= normal * impulse * inverseB;
						a.setVelocity(Rendering::FromGLM(va));
						b.setVelocity(Rendering::FromGLM(vb));
						if (inverseA > 0)
							a.m_ProjectedPosition =
								a.m_previousPosition +
								Rendering::FromGLM(moveA * time + va * static_cast<float>(i_delta) * (1 - time));
						if (inverseB > 0)
							b.m_ProjectedPosition =
								b.m_previousPosition +
								Rendering::FromGLM(moveB * time + vb * static_cast<float>(i_delta) * (1 - time));
					}
				}
				a.m_collisionTime = b.m_collisionTime = time * static_cast<float>(i_delta);
				if (a.m_collisionCallBack)
					a.m_collisionCallBack->HandleCollision(a, b, a.m_collisionTime, Rendering::FromGLM(normal));
				if (a.getState() != ActorState_Dead && b.getState() != ActorState_Dead && b.m_collisionCallBack)
					b.m_collisionCallBack->HandleCollision(b, a, b.m_collisionTime, Rendering::FromGLM(-normal));
			}
		}
	}
} // namespace MyEngine
