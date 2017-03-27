#include "RenderingGame.h"

#include "Rendering/MeshRenderer.h"


void RenderingGame::OnStart()
{
	using namespace std;
	Game::OnStart();

	//Material *pMaterial = new Material("Materials/Default.material");
	////pMaterial->Init("Shaders/DefaultVS.shader", "Shaders/DefaultFS.shader");

	//Model *pModel = new Model("Models/test.obj");
	//pModel->SetMaterial(pMaterial);

	//m_rootNode->AttachRenderable(pModel);

	//Model *pModel_2 = new Model("Models/test.obj");
	//pModel_2->SetMaterial(pMaterial);

	//pTestNode = new SceneNode();
	//pTestNode->AttachRenderable(pModel_2);

	//m_rootNode->AddChild(pTestNode);
	//pTestNode->m_position = vec3(80, 0, 0);
	//pTestNode->m_scale = vec3(0.5f, 0.5f, 0.5f);
	
	
	MeshPtr defaultMesh = m_pMeshManager->Load("Default", "Models/test.obj");

	MaterialPtr mat = m_pMaterialManager->GetByName("Default");

	m_pRootNode->AddComponent<MeshRenderer>(mat, defaultMesh);

}


void RenderingGame::Update()
{
	Game::Update();


//	pTestNode->Pitch(0.0003f); pTestNode->Yaw(0.0003f);






}


void RenderingGame::OnDestroy()
{

}