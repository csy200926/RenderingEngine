#include "GameWorld.h"
#include "PlayerController.h"
#include "Engine/Collision.h"
#include "Engine/MessageManager.h"
#include "Engine/Director.h"
#include "Rendering/MeshManager.h"
#include "Rendering/MaterialManager.h"
#include "Rendering/TextureManager.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/SpriteRenderer.h"
#include "Rendering/Lighting.h"
#include "Platform/Log.h"
#include <algorithm>
namespace
{
	class SpinController : public MyEngine::IActorController
	{
	  public:
		void UpdateActor(MyEngine::Actor &i_actor, bool &, double i_delta) override
		{
			i_actor.Yaw(static_cast<float>(i_delta) * 20);
		}
	};
	class OrbitNode : public Rendering::SceneNode
	{
	  public:
		OrbitNode() : SceneNode("OrbitNode") {}
		void Update(double i_delta) override { Yaw(static_cast<float>(i_delta) * 35); }
	};
} // namespace
GameWorld::GameWorld(const MyEngine::EngineSettings &i_settings) : m_rendering(i_settings.m_scene != "game")
{
	m_playerController = std::make_unique<PlayerController>(*this, i_settings.m_autoplay);
	m_spinController = std::make_unique<SpinController>();
	if (i_settings.m_scene == "sponza" || i_settings.m_scene == "sibenik" || i_settings.m_scene == "model")
		CreateModel(i_settings);
	else if (m_rendering)
		CreateRendering();
	else
		CreateGame();
	auto *pMessages = MyEngine::MessageManager::SharedMessageManager();
	pMessages->RegisterMessageHandler("Hit", this);
	pMessages->RegisterMessageHandler("Hit", &m_hitCounter);
}
void GameWorld::CreateModel(const MyEngine::EngineSettings &i_settings)
{
	using namespace MyEngine;
	using namespace Rendering;
	std::string path = i_settings.m_scene == "sponza"	 ? "Models/Sponza/sponza.obj"
					   : i_settings.m_scene == "sibenik" ? "Models/Sibenik/sibenik.obj"
														 : i_settings.m_model;
	auto mesh = MeshManager::GetInstance()->Load("LargeScene", path);
	glm::vec3 minimum(1e30f), maximum(-1e30f);
	for (auto &vertex : mesh->m_vertices)
	{
		minimum = glm::min(minimum, vertex._position);
		maximum = glm::max(maximum, vertex._position);
	}
	auto dimensions = maximum - minimum;
	float longest = std::max({dimensions.x, dimensions.y, dimensions.z});
	if (longest < 1e-6f)
		throw std::runtime_error("Model has no spatial extent");
	float scale = 40.f / longest;
	glm::vec3 origin((minimum.x + maximum.x) * .5f, minimum.y, (minimum.z + maximum.z) * .5f);
	int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(i_settings.m_instances))));
	float spacing = 44.f;
	for (int i = 0; i < i_settings.m_instances; ++i)
	{
		ActorPtr actor(new Actor("Imported scene " + std::to_string(i)));
		actor->m_scale = glm::vec3(scale);
		auto offset = glm::vec3((i % side - (side - 1) * .5f) * spacing, 0, (i / side - (side - 1) * .5f) * spacing);
		actor->m_position = FromGLM(-origin * scale + offset);
		actor->AddComponent<MeshRenderer>(mesh);
		addActor(actor);
	}
	ActorPtr sun(new Actor("Large scene sunlight"));
	auto *light = sun->AddComponent<DirectionalLight>();
	light->m_direction = glm::vec3(-.3f, -1, -.25f);
	light->SetColor(glm::vec3(2.5f, 2.3f, 2.f));
	addActor(sun);
	for (int i = 0; i < 3 && i_settings.m_instances == 1; ++i)
	{
		ActorPtr lamp(new Actor("Interior fill"));
		lamp->m_position = Vector3((i - 1) * 9.f, 4, 0);
		auto *point = lamp->AddComponent<PointLight>();
		point->SetColor(glm::vec3(1, .85f, .7f));
		point->SetIntensity(1.2f);
		addActor(lamp);
	}
	auto *director = Director::sharedDirector();
	if (i_settings.m_instances > 1)
		director->GetCamera().SetLookAt(glm::vec3(side * 20.f, side * 25.f, side * 38.f), glm::vec3(0, 3, 0),
										glm::vec3(0, 1, 0));
	else
		director->GetCamera().SetLookAt(glm::vec3(-13, 2.5f, 0), glm::vec3(5, 4, 0), glm::vec3(0, 1, 0));
	director->SetCameraProjection(.05f, side * 150.f);
	Platform::Log(
		"Large scene ready: " + path + ", instances=" + std::to_string(i_settings.m_instances) + ", total-triangles=" +
		std::to_string(static_cast<uint64_t>(mesh->m_indices.size() / 3) * i_settings.m_instances) +
		", draws-per-frame=" + std::to_string(mesh->m_subMeshes.size() * i_settings.m_instances) +
		", unique-geometry-MiB=" +
		std::to_string((mesh->m_vertices.size() * sizeof(Vertex) + mesh->m_indices.size() * sizeof(unsigned)) /
					   1048576.0));
}
GameWorld::~GameWorld()
{
	auto *pMessages = MyEngine::MessageManager::SharedMessageManager();
	pMessages->UnregisterMessageHandler("Hit", this);
	pMessages->UnregisterMessageHandler("Hit", &m_hitCounter);
	Platform::Log("GameWorld released: hits=" + std::to_string(m_hits) +
				  ", second-subscriber=" + std::to_string(m_hitCounter.m_count));
}
void GameWorld::CreateGame()
{
	using namespace MyEngine;
	auto logo = addActor("Images/Game/Logo.png", 610, 74, false);
	logo->setPosition(Vector3(0, 225, 0));
	auto player = addActor("Images/Game/player.png", 55, 55, false);
	player->addName("Player");
	player->setPosition(Vector3(-300, -165, 0));
	auto gun = addActor("Images/Game/Gun.png", 60, 45, false);
	gun->addName("Gun");
	gun->setPosition(Vector3(-265, -160, 0));
	gun->addController(m_playerController.get());
	auto floor = addTriggerBox(400, 5, 5, false);
	floor->setPosition(Vector3(0, -240, 0));
	floor->m_CollisionID = Collision::GetCollisionID("Wall");
	floor->m_CollisionLogicalResponseIDMask = Collision::GetCollisionMask("Ball");
	auto wall = addTriggerBox(5, 300, 5, false);
	wall->setPosition(Vector3(410, 0, 0));
	wall->m_CollisionID = Collision::GetCollisionID("Wall");
	wall->m_CollisionLogicalResponseIDMask = Collision::GetCollisionMask("Ball");
	SpawnEnemy(80);
	Platform::Log("Game scene: original player/gun/enemy/projectile assets, W/S aim, J/Space fire, K super");
}
void GameWorld::SpawnEnemy(float i_x)
{
	using namespace MyEngine;
	auto enemy = addActor("Images/Game/Slim.png", 48, 48, false);
	enemy->setClassType("Enemy");
	enemy->setPosition(Vector3(i_x, -160, 0));
	enemy->setVelocity(Vector3(-45, 0, 0));
	enemy->setLifeSpan(14);
	enemy->m_collisionEnabled = true;
	enemy->isTriggerBox = true;
	enemy->m_CollisionID = Collision::GetCollisionID("Enemy");
}
MyEngine::ActorPtr GameWorld::shootProjectileAt(const MyEngine::Vector3 &i_position)
{
	using namespace MyEngine;
	auto projectile = addActor("Images/Game/PlayerBullet.png", 20, 20, false);
	projectile->setClassType("Projectile");
	projectile->setPosition(i_position);
	projectile->setLifeSpan(5);
	projectile->isTriggerBox = true;
	projectile->m_collisionEnabled = true;
	projectile->m_CollisionID = Collision::GetCollisionID("Ball");
	projectile->m_CollisionLogicalResponseIDMask =
		Collision::GetCollisionMask("Enemy") | Collision::GetCollisionMask("Wall");
	projectile->m_collisionCallBack = &m_playerController->m_bulletCollision;
	return projectile;
}
MyEngine::ActorPtr GameWorld::useSuperWeapon()
{
	auto projectile = shootProjectileAt(MyEngine::Vector3(-265, -160, 0));
	projectile->m_scale = glm::vec3(3);
	projectile->setVelocity(MyEngine::Vector3(450, 0, 0));
	return projectile;
}
void GameWorld::handleMessage(MyEngine::Actor &i_actor, MyEngine::IIsA *)
{
	i_actor.getsKilled();
	++m_hits;
	auto effect = addActor("Images/Game/ghost.png", 42, 42, false);
	effect->setPosition(*i_actor.getPosition());
	effect->setVelocity(MyEngine::Vector3(0, 60, 0));
	effect->setLifeSpan(.6f);
	Platform::Log("Hit message: enemy destroyed, effect spawned; hits=" + std::to_string(m_hits));
}
void GameWorld::CreateRendering()
{
	using namespace MyEngine;
	using namespace Rendering;
	auto *pMeshes = MeshManager::GetInstance();
	auto *pMaterials = MaterialManager::GetInstance();
	auto *pTextures = TextureManager::GetInstance();
	auto cube = pMeshes->GetByName("Default");
	auto texture = pTextures->Load("Default", "Images/Rendering/Wood.gif");
	pMaterials->Load("Default", "Materials/Default.material");
	for (int i = 0; i < 5; ++i)
	{
		auto material = pMaterials->Load("PBR" + std::to_string(i),
										 i == 0 ? "Materials/Wood.material" : "Materials/Default.material");
		if (i > 0)
		{
			material->m_color *= glm::vec4(.85f, .23f + .12f * i, .1f, 1);
			material->m_metallic = glm::clamp(material->m_metallic + .2f * i, 0.f, 1.f);
			material->m_roughness = glm::clamp(material->m_roughness + .18f * (i - 2), .045f, 1.f);
		}
		if (i == 0)
			material->SetTexture(texture);
		ActorPtr actor(new Actor("PBR Cube"));
		actor->m_position = Vector3((i - 2) * 1.7f, .6f, 0);
		actor->m_scale = glm::vec3(1.2f);
		actor->AddComponent<MeshRenderer>(material, cube);
		actor->addController(m_spinController.get());
		addActor(actor);
	}
	auto floorMaterial = pMaterials->Create("Floor");
	floorMaterial->SetColor(glm::vec3(.2f, .25f, .3f));
	floorMaterial->m_roughness = .8f;
	ActorPtr floor(new Actor("Floor"));
	floor->m_position = Vector3(0, -.25f, 0);
	floor->m_scale = glm::vec3(12, .2f, 8);
	floor->AddComponent<MeshRenderer>(floorMaterial, cube);
	addActor(floor);
	auto model = pMeshes->Load("Default", "Models/test.obj");
	glm::vec3 minimum(1e30f), maximum(-1e30f);
	for (auto &vertex : model->m_vertices)
	{
		minimum = glm::min(minimum, vertex._position);
		maximum = glm::max(maximum, vertex._position);
	}
	auto size = maximum - minimum;
	float fit = 2.f / std::max({size.x, size.y, size.z});
	for (auto &vertex : model->m_vertices)
		vertex._position = (vertex._position - (minimum + maximum) * .5f) * fit;
	auto modelMat = pMaterials->Create("Teapot");
	modelMat->SetColor(glm::vec3(.15f, .55f, .85f));
	modelMat->m_metallic = .7f;
	modelMat->m_roughness = .2f;
	ActorPtr teapot(new Actor("Original imported model"));
	teapot->m_position = Vector3(0, 1.3f, -2.5f);
	teapot->AddComponent<MeshRenderer>(modelMat, model);
	addActor(teapot);
	// Exercise the original FBX import path with a retained model as well.
	auto fbx = pMeshes->Load("OriginalFBX", "Models/kukri_low.FBX");
	minimum = glm::vec3(1e30f);
	maximum = glm::vec3(-1e30f);
	for (auto &vertex : fbx->m_vertices)
	{
		minimum = glm::min(minimum, vertex._position);
		maximum = glm::max(maximum, vertex._position);
	}
	size = maximum - minimum;
	fit = 2.f / std::max({size.x, size.y, size.z});
	for (auto &vertex : fbx->m_vertices)
		vertex._position = (vertex._position - (minimum + maximum) * .5f) * fit;
	ActorPtr knife(new Actor("Original FBX"));
	knife->m_position = Vector3(3, 1.5f, -2.5f);
	knife->AddComponent<MeshRenderer>(modelMat, fbx);
	knife->addController(m_spinController.get());
	addActor(knife);
	ActorPtr sun(new Actor("Sun"));
	sun->AddComponent<DirectionalLight>()->SetColor(glm::vec3(1.4f, 1.25f, 1.1f));
	addActor(sun);
	for (int i = 0; i < 2; ++i)
	{
		ActorPtr light(new Actor("Point light"));
		light->m_position = Vector3(i ? -4.f : 4.f, 3, 3);
		auto *pLight = light->AddComponent<PointLight>();
		pLight->SetColor(i ? glm::vec3(.1f, .6f, 1) : glm::vec3(1, .3f, .1f));
		pLight->SetIntensity(3);
		addActor(light);
	}
	ActorPtr parent(new Actor("Hierarchy"));
	parent->m_position = Vector3(-3, 1.5f, -2.5f);
	auto *pOrbit = new OrbitNode();
	parent->AddChild(pOrbit);
	auto *pChild = new SceneNode("Child mesh");
	pChild->m_position = Vector3(1, 0, 0);
	pChild->m_scale = glm::vec3(.35f);
	pChild->AddComponent<MeshRenderer>(modelMat, cube);
	pOrbit->AddChild(pChild);
	addActor(parent);
	auto logo = addActor("Images/Game/Logo.png", 350, 43, false);
	logo->setPosition(Vector3(0, 265, 0));
	auto icon = addActor("Images/Game/player.png", 42, 42, false);
	icon->setPosition(Vector3(-350, -250, 0));
	Platform::Log(
		"Rendering scene: shared PBR, irradiance, OBJ/FBX import, lights, hierarchy and original alpha sprites");
}
void GameWorld::update(double i_delta)
{
	if (m_rendering)
		return;
	counter += static_cast<float>(i_delta);
	m_elapsed += static_cast<float>(i_delta);
	if (counter >= 1.4f)
	{
		counter = 0;
		SpawnEnemy(330);
	}
}
