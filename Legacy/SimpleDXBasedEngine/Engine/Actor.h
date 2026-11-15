#pragma once
#include "TemplateArray.h"
#include "TemplateArray.cpp"
#include "MemoryPool.h"
#include "HashedString.h"
#include "ICollisionCallBack.h"
#include "IIsA.h"
#include "Cheesy.h"
#include "Sprite.h"
#include <Material.h>
#include <Mesh.h>

#define DEFAULT_MEMORYPOOL_SIZE 25
#define PLAYER 1001



namespace MyEngine{



	typedef enum
	{
		ActorState_Dead = 0,
		ActorState_Alive,
		ActorState_Frosen,
		ActorState_AvatarState
	}ActorState;


	class Vector3;
	class IActorController;
	
	struct Center
	{
		float				x;
		float				y;
		float				z;
	} ;

	struct AABB
	{
		Center			center;
	
		float				HalfX;
		float				HalfY;
		float				HalfZ;
	} ;


	class Actor : public IIsA
	{




	private:

		float lifeSpan;

		TemplateArray<IActorController *> _pActorControllers;
		IActorController *_pActorController;

		Vector3 *_pPosition;
		Vector3 *_pVelocity;
		Vector3 *_pAcceleration;

		Vector3 *m_rotation;

		ActorState m_ActorState;
		int m_tag;

		const char * m_pClassType;
		HashedString * m_pName;

		float m_mass;
		
		void init();

	public:
		bool is3DObj;

		
		HashedString materialName;
		HashedString meshName;

		/***********************collision*************/
		bool isTriggerBox;

		float m_collisionTime;
		ICollisionCallBack *m_collisionCallBack;
		unsigned int	m_CollisionLogicalResponseIDMask;
		unsigned int	m_CollisionID;
		//-----boundingBox--------
		AABB boundingBox;

		void projectMove(const double delta);
		void finalizeMove();
		Vector3	*m_ProjectedPosition;
		Vector3	*m_ProjectedVelocity;
		/***********************collision*************/


		// MemoryPool
		static void setMemoryPool( MemoryPool *i_pMemoryPool );
		static void releaseMemoryPool( );
		Cheesy::Sprite* m_pSprite;


		// Overrides New Delete                   a new must match a delete
		void *operator new(const size_t i_size);
		void *operator new(const size_t i_size,const char* i_file,const int line );
		void operator delete(void *ptr);
		void operator delete(void *ptr,const char* i_file,const int line );

		//Actor();
		Actor(float Bx,float By,float Bz,bool is3DObj);
		Actor(const char * textureName,const float width,const float height,bool is3DObj);
		~Actor(void);

		inline Vector3* getAcceleration(); 
		inline void setAcceleration(const Vector3 & i_Acceleration); 

		inline Cheesy::Sprite * getSprite();
		
		inline void setPosition(const Vector3 & position);
		inline void setVelocity(const Vector3 & velocity);
		inline void addController(IActorController *controller);
		inline Vector3* getPosition() const;

		inline ActorState getState() const;
		inline void getsKilled();

		inline void setTag(const int tag);
		inline int getTag() const;

		inline void setMass(const float Mass);
		inline float getMass() const;

		inline Vector3* Actor::getRotation();
		inline void setRotation(const Vector3 & i_rotation);
		inline Vector3* getVelocity(); 
		void update(double delta);
		virtual void act(double delta){};

		inline void setClassType(const char *);
		inline const char * getClassType();

		inline void addName(const char *);
		inline unsigned int getName();
		char texture;

		inline void setLifeSpan( float time );
	};


}
#include "Actor.inl"