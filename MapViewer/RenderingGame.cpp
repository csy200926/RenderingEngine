#include "RenderingGame.h"

#include "Rendering/MeshRenderer.h"
#include "Rendering/Lighting.h"

#include "Rendering/MaterialManager.h"
#include "Rendering/TextureManager.h"
#include "Rendering/MeshManager.h"


void RenderingGame::OnStart()
{
	using namespace glm;
	using namespace std;
	Game::OnStart();
	//return;
	//LoadScene("Scene.lua");
	//return;
	m_pTextureManager->Load("terrain", "Images/terrain.png");
		MeshPtr tableMesh = m_pMeshManager->Load("Table", "Models/sponza.obj");
	
	MaterialPtr defaultMat = m_pMaterialManager->GetByName("Default");//GetByName("Deferred_first");
	
	TexturePtr woodTexture = m_pTextureManager->Load("Wood", "Images/defaultTex.jpg");
	defaultMat->SetTexture(woodTexture);
	
	MeshRenderer *pMeshRenderer = m_pRootNode->AddComponent<MeshRenderer>(defaultMat, tableMesh);
	
	//Point lights test
	{		
		SceneNode *pDirLightNode = new SceneNode("DirLight");
		m_pRootNode->AddChild(pDirLightNode);

		DirectionalLight* pDirLight = pDirLightNode->AddComponent<DirectionalLight>();
		pDirLight->SetColor(vec3(0.0f, 1.0f, 0.0f));
		pDirLightNode->m_position = glm::vec3(1, 0, 0);


		//SceneNode *pLightNode_1 = new SceneNode("Light1");
		//m_pRootNode->AddChild(pLightNode_1);

		//PointLight* pPointLight_1 = pLightNode_1->AddComponent<PointLight>();
		//pPointLight_1->SetColor(vec3(0.0f, 1.0f, 0.0f));
		//pLightNode_1->m_position = glm::vec3(0, -5, 0);


		//SceneNode *pLightNode_2 = new SceneNode("Light2");
		//m_pRootNode->AddChild(pLightNode_2);

		//PointLight* pPointLight_2 = pLightNode_2->AddComponent<PointLight>();
		//pPointLight_2->SetColor(vec3(0.0f, 0.0f, 1.0f));
		//pLightNode_2->m_position = glm::vec3(15, -5, 0);
	}



}


void RenderingGame::Update()
{
	Game::Update();


//	pTestNode->Pitch(0.0003f); pTestNode->Yaw(0.0003f);






}


void RenderingGame::OnDestroy()
{

}