#include "RenderingGame.h"

#include "Rendering/MeshRenderer.h"
#include "Rendering/Lighting.h"

void RenderingGame::OnStart()
{
	using namespace std;
	Game::OnStart();


	
	MeshPtr defaultMesh = m_pMeshManager->GetByName("Default");//Load("Default", "Models/box.obj");
	
	MaterialPtr defaultMat = m_pMaterialManager->GetByName("Default");//GetByName("Deferred_first");
	TexturePtr woodTexture = m_pTextureManager->Load("Wood", "Images/Wood.gif");
	defaultMat->SetTexture(woodTexture);

	MeshRenderer *pMeshRenderer = m_pRootNode->AddComponent<MeshRenderer>(defaultMat, defaultMesh);

	//Point lights test
	{
		SceneNode *pLightNode_1 = new SceneNode("Light1");
		m_pRootNode->AddChild(pLightNode_1);

		PointLight* pPointLight_1 = pLightNode_1->AddComponent<PointLight>();
		pPointLight_1->SetColor(vec3(0.0f, 1.0f, 0.0f));
		pLightNode_1->m_position = glm::vec3(0, -5, 0);


		SceneNode *pLightNode_2 = new SceneNode("Light2");
		m_pRootNode->AddChild(pLightNode_2);

		PointLight* pPointLight_2 = pLightNode_2->AddComponent<PointLight>();
		pPointLight_2->SetColor(vec3(0.0f, 0.0f, 1.0f));
		pLightNode_2->m_position = glm::vec3(15, -5, 0);
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