#include "Collision.h"
#include "Matrix.h"
#include "Vector4.h"
#include "Actor.h"
#include "NamedBitSet.h"
#include "HashedString.h"
#include "Profiling.h"
#include "ScopedTimer.h"



namespace MyEngine{

	NamedBitSet<unsigned int> CollisionIDs;

	unsigned int  Collision::GetCollisionID( const HashedString & i_TypeHash )
	{
		return CollisionIDs.GetBitIndex( i_TypeHash );
	}


	unsigned int Collision::GetCollisionMask( const HashedString & i_TypeHash )
	{
		return CollisionIDs.GetBitMask(i_TypeHash); 
	}

	unsigned int Collision::GetCollisionMask( unsigned int i_ID )
	{
		return CollisionIDs.GetBitMask(i_ID);
	}


	Collision::Collision(void)
	{
		

	}


	Collision::~Collision(void)
	{
		clear();

	}

	void Collision::clear(void)
	{
		
		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
			
			Actors.remove(Actors[i]);
		}
	}

	bool checkCases(float left, float right,float point, float velocity,float &tExit,float &tEnter)
	{
		
		float xMin = left;
		float xMax = right;


		//DEBUG_PRINT("%f,%f\n",xMin,xMax);

		if (velocity - 0 <= 0.1f)
		{
			if (xMin <= point && point <=  xMax)
			{
				tEnter = 0.0f;
				tExit = 0.016f;
				return true;
			}else
			{
				return false;
			}
		}


		if (xMin <=point && point  <=  xMax)
		{
			tEnter = 0.0f;

			if (velocity < 0)
			{
				tExit = (point - xMin) /  -velocity;
			}else
			{
				tExit = (xMax - point) / velocity;
			}
		}else if (point > xMax)
		{
			if (velocity > 0)
			{
				return false;
			}else
			{
				tEnter = ( point - xMax ) / -velocity;
				tExit = ( point - xMin ) / -velocity;
			}
		}else if (point< xMin)
		{

			if (velocity < 0)
			{
				return false;
			}else
			{
				tExit = - ( point - xMax ) / velocity;
				tEnter = -( point - xMin ) / velocity;
			}

		}

		if (tEnter >= 0.016f)
		{
			return false;
		}


		return true;
	}


	bool CheckOOBBIntersection( const AABB & i_BoxA, const Vector3 & i_VelocityA, const Matrix & i_ObjAtoWorld, const AABB & i_BoxB, const Vector3 & i_VelocityB, const Matrix & i_ObjBtoWorld, float &o_overlaptime)
	{
		float tFirstExit = 0.0f;
		float tLastEnter = 0.0f;

		float tExit = 1.0f;
		float tEnter = 0.0f;


		//A in B
		{
			Matrix ObjBtoWorldInverse(i_ObjBtoWorld);
			ObjBtoWorldInverse.Inverse();

			Matrix Mab = ObjBtoWorldInverse * i_ObjAtoWorld;

			// Transform both Objects velocities from World CS to Object B's CS
			Vector4 velocityA4(i_VelocityA.getX(),i_VelocityA.getY(),i_VelocityA.getZ(),0); // D3DXVec3TransformNormal(&velocityA,&i_VelocityA,&Mab);
			velocityA4 = velocityA4 * i_ObjBtoWorld;
			Vector4 velocityB4(i_VelocityB.getX(),i_VelocityB.getY(),i_VelocityB.getZ(),0); // D3DXVec3TransformNormal(&velocityA,&i_VelocityA,&Mab);
			velocityB4 = velocityB4 * i_ObjBtoWorld;

	/*
			// Project A's BB extents (width and height) onto B's axis
			Vector4 Nx(1.0f,0.0f,0.0f,1.0f); 
			Nx = Nx * Mab;
			Vector3 Nx3(Nx.getX(),Nx.getY(),Nx.getZ());

			Vector4 Ny(0.0f,1.0f,0.0f,1.0f); 
			Ny = Ny * Mab;
			Vector3 Ny3(Ny.getX(),Ny.getY(),Ny.getZ());

			Vector3 Hw = Vector3(i_BoxA.HalfX,0.0f,0.0f);
			Vector3 Hh = Vector3(0.0f,i_BoxA.HalfY,0.0f);

			Vector3 x = Vector3(1.0f,0.0f,0.0f);
			Vector3 y = Vector3(0.0f,1.0f,0.0f);

			Vector3 a(Hw * Nx3.dot(x));Vector3 b(Hh * Ny3.dot(x));
			float extentsX = a.length() + b.length();

			Vector3 c(Hh * Ny3.dot(y));Vector3 d(Hw * Nx3.dot(y));
			float extentsY = c.length() + d.length();
*/			// Move A's BB center to Object B's CS
			Vector4 shrinkedBoxA4 = Vector4(0.0f,0.0f,0.0f,0.0f);
			// = shrinkedBoxA4 * i_ObjAtoWorld;
			shrinkedBoxA4 = shrinkedBoxA4 * Mab;
//			DEBUG_PRINT("%f,%f\n",shrinkedBoxA4.getX(),shrinkedBoxA4.getY());
			Vector3 shrinkedBoxA(shrinkedBoxA4.getX(),shrinkedBoxA4.getY(),0);

			Vector4 A(-i_BoxA.HalfX,i_BoxA.HalfY,0,0);
			Vector4 B(i_BoxA.HalfX,i_BoxA.HalfY,0,0);
			Vector4 C(i_BoxA.HalfX,-i_BoxA.HalfY,0,0);
			Vector4 D(-i_BoxA.HalfX,-i_BoxA.HalfY,0,0);

			A = A * Mab;
			B = B * Mab;
			C = C * Mab;
			D = D * Mab;
			// x
			float extentsX = 0;
			if (abs( A.getX()- shrinkedBoxA.getX()) > extentsX)
			{
				extentsX = abs( A.getX()- shrinkedBoxA.getX());
			}
			if (abs( B.getX()- shrinkedBoxA.getX()) > extentsX)
			{
				extentsX = abs( B.getX()- shrinkedBoxA.getX());
			}
			if (abs(C.getX()- shrinkedBoxA.getX()) > extentsX)
			{
				extentsX = abs(C.getX()- shrinkedBoxA.getX());
			}
			if (abs( D.getX()- shrinkedBoxA.getX()) > extentsX)
			{
				extentsX = abs( D.getX()- shrinkedBoxA.getX());
			}

			float extentsY = 0;
			if (abs( A.getY()- shrinkedBoxA.getY()) > extentsY)
			{
				extentsY = abs( A.getY()- shrinkedBoxA.getY());
			}
			if (abs( B.getY()- shrinkedBoxA.getY()) > extentsY)
			{
				extentsY = abs( B.getY()- shrinkedBoxA.getY());
			}
			if (abs(C.getY()- shrinkedBoxA.getY()) > extentsY)
			{
				extentsY = abs(C.getY()- shrinkedBoxA.getY());
			}
			if (abs( D.getY()- shrinkedBoxA.getY()) > extentsY)
			{
				extentsY = abs( D.getY()- shrinkedBoxA.getY());
			}
		
			float extentsZ = 0;
			if (abs( A.getZ()- shrinkedBoxA.getZ()) > extentsZ)
			{
				extentsZ = abs( A.getZ()- shrinkedBoxA.getZ());
			}
			if (abs( B.getZ()- shrinkedBoxA.getZ()) > extentsZ)
			{
				extentsZ = abs( B.getY()- shrinkedBoxA.getZ());
			}
			if (abs(C.getZ()- shrinkedBoxA.getZ()) > extentsZ)
			{
				extentsZ = abs(C.getZ()- shrinkedBoxA.getZ());
			}
			if (abs( D.getZ()- shrinkedBoxA.getZ()) > extentsZ)
			{
				extentsZ = abs( D.getZ()- shrinkedBoxA.getZ());
			}
		
			// Expand B's BB by A's BB size
			AABB extentedBoxB = AABB(i_BoxB);
			extentedBoxB.HalfX += extentsX;
			extentedBoxB.HalfY += extentsY;
			extentedBoxB.HalfZ += extentsZ;
			//DEBUG_PRINT("%f  \n",extentsX);

			// Calculate A's velocity relative to B
			velocityA4 = velocityA4 - velocityB4;
			Vector3 velocityA = Vector3(velocityA4.getX(),velocityA4.getY(),velocityA4.getZ());
			// i.e. Subtract B's velocity from A
	
			// Now we have a ray from A's BB center in B's CS to A's BB center + A's velocity in B
			// that we can check against B's expanded BB (one axis at a time)

	
			
			// x axis
			if(!checkCases(extentedBoxB.center.x - extentedBoxB.HalfX,extentedBoxB.center.x + extentedBoxB.HalfX,shrinkedBoxA.getX(),velocityA.getX(),tExit,tEnter))
			{
				return false;
			}

			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;
			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}
			
			// y axis
			if(!checkCases(extentedBoxB.center.y - extentedBoxB.HalfY,extentedBoxB.center.y + extentedBoxB.HalfY,shrinkedBoxA.getY(),velocityA.getY(),tExit,tEnter))
			{
				return false;
			}
			
			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;
			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}

			// z axis
			if(!checkCases(extentedBoxB.center.z - extentedBoxB.HalfZ,extentedBoxB.center.z + extentedBoxB.HalfZ,shrinkedBoxA.getZ(),velocityA.getZ(),tExit,tEnter))
			{
				return false;
			}
			
			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;
			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}
		}
		
		//B in A
		{
			Matrix ObjAtoWorldInverse(i_ObjAtoWorld);
			ObjAtoWorldInverse.Inverse();

			Matrix Mba = ObjAtoWorldInverse * i_ObjBtoWorld;

			// Transform both Objects velocities from World CS to Object B's CS
			Vector4 velocityA4(i_VelocityA.getX(),i_VelocityA.getY(),i_VelocityA.getZ(),0); // D3DXVec3TransformNormal(&velocityA,&i_VelocityA,&Mab);
			velocityA4 = velocityA4 * i_ObjAtoWorld;
			Vector4 velocityB4(i_VelocityB.getX(),i_VelocityB.getY(),i_VelocityB.getZ(),0); // D3DXVec3TransformNormal(&velocityA,&i_VelocityA,&Mab);
			velocityB4 = velocityB4 * i_ObjAtoWorld;


			Vector4 shrinkedBoxB4 = Vector4(0.0f,0.0f,0.0f,0.0f);
			shrinkedBoxB4 = shrinkedBoxB4 * Mba;

			Vector3 shrinkedBoxB(shrinkedBoxB4.getX(),shrinkedBoxB4.getY(),0);

			Vector4 A(-i_BoxB.HalfX,i_BoxB.HalfY,0,0);
			Vector4 B(i_BoxB.HalfX,i_BoxB.HalfY,0,0);
			Vector4 C(i_BoxB.HalfX,-i_BoxB.HalfY,0,0);
			Vector4 D(-i_BoxB.HalfX,-i_BoxB.HalfY,0,0);

			A = A * Mba;
			B = B * Mba;
			C = C * Mba;
			D = D * Mba;
			// x
			float extentsX = 0;
			if (abs( A.getX()- shrinkedBoxB.getX()) > extentsX)
			{
				extentsX = abs( A.getX()- shrinkedBoxB.getX());
			}
			if (abs( B.getX()- shrinkedBoxB.getX()) > extentsX)
			{
				extentsX = abs( B.getX()- shrinkedBoxB.getX());
			}
			if (abs(C.getX()- shrinkedBoxB.getX()) > extentsX)
			{
				extentsX = abs(C.getX()- shrinkedBoxB.getX());
			}
			if (abs( D.getX()- shrinkedBoxB.getX()) > extentsX)
			{
				extentsX = abs( D.getX()- shrinkedBoxB.getX());
			}

			float extentsY = 0;
			if (abs( A.getY()- shrinkedBoxB.getY()) > extentsY)
			{
				extentsY = abs( A.getY()- shrinkedBoxB.getY());
			}
			if (abs( B.getY()- shrinkedBoxB.getY()) > extentsY)
			{
				extentsY = abs( B.getY()- shrinkedBoxB.getY());
			}
			if (abs(C.getY()- shrinkedBoxB.getY()) > extentsY)
			{
				extentsY = abs(C.getY()- shrinkedBoxB.getY());
			}
			if (abs( D.getY()- shrinkedBoxB.getY()) > extentsY)
			{
				extentsY = abs( D.getY()- shrinkedBoxB.getY());
			}
		
			AABB extentedBoxA = AABB(i_BoxA);
			extentedBoxA.HalfX += extentsX;
			extentedBoxA.HalfY += extentsY;

			velocityB4 = velocityB4 - velocityA4;
			Vector3 velocityB = Vector3(velocityB4.getX(),velocityB4.getY(),velocityB4.getZ());


			// x axis
			if(!checkCases(extentedBoxA.center.x - extentedBoxA.HalfX,extentedBoxA.center.x + extentedBoxA.HalfX,shrinkedBoxB.getX(),velocityB.getX(),tExit,tEnter))
			{
				return false;
			}

			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;
	

			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}

			// y axis
			if(!checkCases(extentedBoxA.center.y - extentedBoxA.HalfY,extentedBoxA.center.y + extentedBoxA.HalfY,shrinkedBoxB.getY(),velocityB.getY(),tExit,tEnter))
			{
				return false;
			}
			
			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;


			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}

			// z axis
			if(!checkCases(extentedBoxA.center.z - extentedBoxA.HalfZ,extentedBoxA.center.z + extentedBoxA.HalfZ,shrinkedBoxB.getZ(),velocityB.getZ(),tExit,tEnter))
			{
				return false;
			}
			
			if (tEnter > tLastEnter)
			{
				tLastEnter = tEnter;
			}
			if (tExit < tFirstExit)
			{
				tFirstExit = tExit;
			}
		}

		o_overlaptime = tLastEnter;
		
		
		if (tLastEnter < tFirstExit)
		{

			return false;
		}
		
		return true;
	}




	void Collision::update(const double delta)
	{					
		#if defined( ENABLE_PROFILING )
		ScopedTimer collisionRecording("Collision");
		#endif
		for (int i = 0; i < Actors.len; i++)
		{
			MyEngine::MySharedPointer<MyEngine::Actor> A = Actors[i];
			
			if (A->getState() == ActorState_Dead )
			{
				Actors.remove(A);
				continue;
			}


			for (int j = i + 1; j < Actors.len; j++)
			{
				MyEngine::MySharedPointer<MyEngine::Actor> B = Actors[j];

				if (B->getState() == ActorState_Dead )
				{
					Actors.remove(B);
				
					continue;
				}


				bool ACollidableToB = (A->m_CollisionLogicalResponseIDMask & GetCollisionMask( B->m_CollisionID)) != 0;
				bool BCollidableToA = (B->m_CollisionLogicalResponseIDMask & GetCollisionMask( A->m_CollisionID)) != 0;

				if (ACollidableToB | BCollidableToA)
				{
					Matrix	Obj1Rotation, Obj1Translation, Obj1ToWorld;
					Matrix	Obj2Rotation, Obj2Translation, Obj2ToWorld;

					Obj1Rotation.CreateRotateZ((float)(DEG_TO_RADIANS(A->getRotation()->getX())));
					Obj1Translation.Translate(A->m_ProjectedPosition->getX(),A->m_ProjectedPosition->getY(),0);
					Obj1ToWorld =  Obj1Translation * Obj1Rotation;//zhu yi shun xu
		
		
					Obj2Rotation.CreateRotateZ((float)(DEG_TO_RADIANS(B->getRotation()->getX())));
					Obj2Translation.Translate(B->m_ProjectedPosition->getX(),B->m_ProjectedPosition->getY(),0);
					Obj2ToWorld =  Obj2Translation * Obj2Rotation;

					float overlapTime = 0.0f;
		
					if (CheckOOBBIntersection(B->boundingBox,*B->getVelocity(),Obj2ToWorld,A->boundingBox,*A->getVelocity(),Obj1ToWorld,overlapTime))
					{
						// m
						float newXA,newXB,newYA,newYB;
						float massA = A->getMass(),massB = B->getMass();

						newXA = ((massA - massB)/ (massA + massB)) * A->getVelocity()->getX() + (2*massB / (massA + massB))* B->getVelocity()->getX();
						newXB = ((massB - massA)/ (massA + massB)) * B->getVelocity()->getX() + (2*massA / (massA + massB))* A->getVelocity()->getX();

						newYA = ((massA - massB)/ (massA + massB)) * A->getVelocity()->getY() + (2*massB / (massA + massB))* B->getVelocity()->getY();
						newYB = ((massB - massA)/ (massA + massB)) * B->getVelocity()->getY() + (2*massA / (massA + massB))* A->getVelocity()->getY();

						// first collision response
						Vector3 v(0.0f,0.0f,0.0f);
						Vector3 pos(0.0f,0.0f,0.0f);
					
						v.setValue(newXA,newYA,0);
	
						
						A->setVelocity(v);
						pos = *A->getPosition() + *A->getVelocity() * overlapTime;
						*A->m_ProjectedPosition = pos;
						
						
						
						v.setValue(newXB,newYB,0);
						
						B->setVelocity(v);
						pos = *B->getPosition() + *B->getVelocity() * overlapTime;
						*B->m_ProjectedPosition = pos;
					
						
				
						if (A->m_collisionCallBack)
							A->m_collisionCallBack->HandleCollision(*A,*B,overlapTime,v);
		
						if (B->m_collisionCallBack)
							B->m_collisionCallBack->HandleCollision(*B,*A,overlapTime,v);
					
					}
				} 
			



			}

		
		

		}



	}










}