#include "Actor.h"
#include "Vector3.h"
#include "IActorController.h"
#include "Extension.h"
#include "AllocationOverride.h"
#include <Texture.h>
//extern struct Cheesy::_MeshData CubeData;
namespace MyEngine{
	
	Cheesy::Texture * CreateTextureFromFile( const char * i_pTextureFilename );

	// MemoryPool Init
	static MemoryPool *pMemoryPool = NULL;
	void Actor::setMemoryPool( MemoryPool *i_pMemoryPool )
	{
		pMemoryPool = i_pMemoryPool;
	}

	void Actor::releaseMemoryPool( )
	{
		if ( pMemoryPool != NULL)
		{
			DEBUG_PRINT("Deleting ActorsMemoryPool!");
			delete pMemoryPool;
		}
	}

	void *Actor::operator new(const size_t i_size,const char* i_file,const int line )
	{
		DEBUG_PRINT("Calling new ( size_t ) with ( %d ) at line:%d, file:%s \n",i_size,line,i_file);

		if ( pMemoryPool == NULL)
		{
			DEBUG_PRINT("Default ActorsMemoryPool Set!");
			pMemoryPool = MemoryPool::create(sizeof(Actor), DEFAULT_MEMORYPOOL_SIZE);
		}

		return pMemoryPool->alloc();
	}

	void *Actor::operator new(const size_t i_size )
	{
		DEBUG_PRINT("Calling new ( size_t ) with ( %d ) \n", i_size);

		if ( pMemoryPool == NULL)
		{
			DEBUG_PRINT("Default MemoryPool Set!");
			pMemoryPool = MemoryPool::create(sizeof(Actor), DEFAULT_MEMORYPOOL_SIZE);
		}

		return pMemoryPool->alloc();
	}
	void Actor::operator delete(void *i_ptr,const char* i_file,const int line )
	{
		DEBUG_PRINT("Calling delete( void * ) on %p \n" , i_ptr);

		pMemoryPool->free(i_ptr);
	}

	void Actor::operator delete(void *i_ptr)
	{
		DEBUG_PRINT("Calling delete( void * ) on %p \n" , i_ptr);

		pMemoryPool->free(i_ptr);
	}



	Actor::Actor(float Bx,float By,float Bz, bool i_is3DObj) : IIsA("Actor") 
	{


		is3DObj = i_is3DObj;

		init();
		isTriggerBox = true;

		boundingBox.center.x = 0.0f;
		boundingBox.center.y = 0.0f;
		boundingBox.center.z = 0.0f;

		boundingBox.HalfX = Bx;
		boundingBox.HalfY = By;
		boundingBox.HalfZ = Bz;


		m_mass = 11000.0f;

		m_pName = NULL;

	}
	Actor::Actor(const char * textureName,const float width,const float height, bool i_is3DObj): IIsA("Actor") 
	{
		
		is3DObj = i_is3DObj;
		init();

		isTriggerBox = false;

		boundingBox.center.x = 0.0f;
		boundingBox.center.y = 0.0f;
		boundingBox.center.z = 0.0f;

		boundingBox.HalfX = width * 0.5f;
		boundingBox.HalfY = height * 0.5f;
		boundingBox.HalfZ = 25.0f;

		if (!is3DObj)
		{
			
			struct Cheesy::Point2D		center( 0.0f, 0.0f );
			struct Cheesy::Point2D		size( width, height );
			struct Cheesy::ColorRGBA	white( 255, 255, 255, 255 );
			Cheesy::UVSet UVs = { Cheesy::UV( 0.0f, 0.0f ), Cheesy::UV( 1.0f, 0.0f ), Cheesy::UV( 0.0f, 1.0f ), Cheesy::UV( 1.0f, 1.0f ) };
		
		
			//*******For Test Only***********

			Cheesy::Texture * pSampleTexture = CreateTextureFromFile(textureName);
			m_pSprite = Cheesy::Sprite::Create( center, 0.1f, size, white, *pSampleTexture, UVs );
			pSampleTexture->Release();
			
			//********************************
		}else
		{
			meshName = "MeshCube";
			materialName = textureName;
		
		}



	}


	void Actor::init()
	{
	
		lifeSpan = -100;// Never automaticlly get destroyed
		m_ProjectedPosition = CNEW Vector3(0.0f,0.0f,0.0f);
		_pPosition = CNEW Vector3(0.0f,0.0f,0.0f);
		_pVelocity = CNEW Vector3(0.0f,0.0f,0.0f);
		_pAcceleration = CNEW Vector3(0.0f,0.0f,0.0f);
		m_rotation = CNEW Vector3(0.0f,0.0f,0.0f);
		//m_rotation = 0.0f;

		_pActorController = NULL;
		m_tag = 0;
		

		m_pName = NULL;

		m_ActorState = ActorState_Alive;

		// Collision
		m_CollisionLogicalResponseIDMask = 0;
		m_CollisionID = 0;
		m_mass = 1.0f;
		m_collisionCallBack = NULL;

		m_collisionTime = 0.0f;
	
	}





	void Actor::projectMove(const double delta)
	{

		*m_ProjectedPosition = *_pPosition + *_pVelocity * (float)delta;

	}

	void Actor::finalizeMove()
	{
		if (!isTriggerBox)
		{
			*_pPosition = *m_ProjectedPosition;
		}
		
	}

	Actor::~Actor(void)
	{

		SAFE_DELETE(m_pName);
		SAFE_DELETE(m_ProjectedPosition);
		SAFE_DELETE(_pPosition);
		SAFE_DELETE(_pVelocity);
		SAFE_DELETE(m_rotation);
		SAFE_DELETE(_pAcceleration);

		if (!isTriggerBox && !is3DObj)
		{
			SAFE_DELETE(m_pSprite);
		}



	}

	void Actor::update( double delta)
	{
		if (getState() == ActorState_Dead )
		{
			return;
		}

		*_pVelocity = *_pVelocity + *_pAcceleration;
		//_pVelocity->setValue( _pVelocity->getX() * 0.99f,_pVelocity->getY() * 0.99f ,0.0f);



		bool isDone = false;

		int i = 0;
		for ( ; i < _pActorControllers.len ; i++)
		{
			
			_pActorControllers[i]->UpdateActor(*this,isDone,delta);
		}

		if( lifeSpan > 0 )
		{
			lifeSpan -= (float)delta;
			if( lifeSpan < 0 )
				getsKilled();
		}

		return;
	}


	
	Cheesy::Texture * CreateTextureFromFile( const char * i_pTextureFilename )
	{
		assert( i_pTextureFilename != NULL );

		FILE * pFile = NULL; 

		errno_t fopenError = fopen_s( &pFile, i_pTextureFilename, "rb" );
		if( fopenError != 0 )
			return NULL;

		assert( pFile != NULL );

		int FileIOError = fseek( pFile, 0, SEEK_END );
		assert( FileIOError == 0 );

		long FileSize = ftell( pFile );
		assert( FileSize >= 0 );

		FileIOError = fseek( pFile, 0, SEEK_SET );
		assert( FileIOError == 0 );

		char * pBuffer = new char[FileSize];
		assert( pBuffer );

		long FileRead = fread( pBuffer, 1, FileSize, pFile );
		assert( FileRead == FileSize );

		fclose( pFile );

		Cheesy::Texture * pTexture = Cheesy::Texture::Create( pBuffer, FileSize );

		delete [] pBuffer;

		return pTexture;
	}
	

}



