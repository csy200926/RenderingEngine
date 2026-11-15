#pragma once

namespace MyEngine{
	class Actor;
	class Vector3;

	class ICollisionCallBack 
	{
	public:

		virtual ~ICollisionCallBack(){};

		virtual void HandleCollision( Actor & i_Me,  Actor & i_CollidedWith, const float i_CollisionTime, const Vector3 & i_CollisionVector ) = 0;
	};

}
