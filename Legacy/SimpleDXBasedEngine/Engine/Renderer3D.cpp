#include "Renderer3D.h"
#include "Actor.h"
#include "D3dx9math.h"
extern struct Cheesy::_MeshData CubeData;
namespace MyEngine{

	Renderer3D::Renderer3D(void): IIsA("Renderer3D")
	{


		Cheesy::Mesh *pMesh = Cheesy::Mesh::Create( CubeData );
		HashedString meshName("MeshCube");
		MyEngine::MySharedPointer<Cheesy::Mesh> cubeMesh(pMesh);
		meshMap.insert(MeshMap::value_type( meshName,cubeMesh) );





		const char * pTextures_1[] = { "image/Slim.png" };
		Cheesy::Material *material_1 = Cheesy::Material::Create( "image/Simple.fx", pTextures_1, 1 );
		MyEngine::MySharedPointer<Cheesy::Material> cubeMaterial_1(material_1);

		const char * pTextures_2[] = { "image/ghost.png" };
		Cheesy::Material *material_2 = Cheesy::Material::Create( "image/Simple.fx", pTextures_2, 1 );
		MyEngine::MySharedPointer<Cheesy::Material> cubeMaterial_2(material_2);

		materialMap.insert(MaterialMap::value_type( "MaterialSlim",cubeMaterial_1) );
		materialMap.insert(MaterialMap::value_type( "MaterialGhost",cubeMaterial_2) );
	}

	void Renderer3D::clear(void)
	{

		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
			//Actors[i].release();
			Actors.remove(Actors[i]);
		}
	}

	Renderer3D::~Renderer3D(void)
	{

		clear();
	}

	void Renderer3D::draw(  )
	{
		using namespace MyEngine;

		struct Cheesy::Point2D	offsetBG( 0.0f, 0.0f );

		// mix map with Actor	
		int i = 0;
		for ( ; i < Actors.len ; i++)
		{
		
			MySharedPointer<Actor> &s = Actors[i];

			if (s->getState() == ActorState_Dead )
			{
				Actors.remove(s);
				continue;
			}
			if(s->isTriggerBox)
				continue;
			
			// Set up location matrix
			D3DXMATRIX LocalToWorld;
			D3DXMatrixTranslation(&LocalToWorld,s->getPosition()->getX(),s->getPosition()->getY(),s->getPosition()->getZ());

			D3DXMATRIX	RotX, RotY ,RotZ;
			D3DXMATRIX  Rotation;
			D3DXMatrixRotationX( &RotX, s->getRotation()->getX() );
			D3DXMatrixRotationY( &RotY, s->getRotation()->getY() );
			D3DXMatrixRotationZ( &RotZ, s->getRotation()->getZ() );

			D3DXMatrixMultiply( &Rotation, &RotX, &RotY);
			D3DXMatrixMultiply( &Rotation ,&Rotation , &RotZ);
			D3DXMatrixMultiply( &LocalToWorld ,&Rotation , &LocalToWorld);

			// Start drawing
			MyEngine::MySharedPointer<Cheesy::Mesh> mesh;
			MyEngine::MySharedPointer<Cheesy::Material> material;

			MeshMap::iterator meshIt = meshMap.find(s->meshName);
			if(meshIt != meshMap.end())
			{
				mesh = meshIt->second;
			}
			
			MaterialMap::iterator materialIt = materialMap.find(s->materialName);
			if(materialIt != materialMap.end())
			{
				material = materialIt->second;
			}
			mesh->Draw(&LocalToWorld ,*(material) );

		
		}

	}

	void Renderer3D::setRenderQueue(MyEngine::Scene *i_pScene  )
	{
		m_pScene = i_pScene;

	}

}