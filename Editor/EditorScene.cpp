#include "EditorScene.h"
#include "Rendering/Terrain.h"
#include "Rendering/MeshManager.h"
#include "Rendering/MeshRenderer.h"
#include "Rendering/Lighting.h"
#include "Rendering/TextureCube.h"
#include "Platform/Paths.h"
#include <algorithm>

namespace Editor
{
	std::string GetModelDisplayName(const std::string &i_path)
	{
		if (i_path == "builtin:cube")
			return "方块 Cube";
		if (i_path == "builtin:quad")
			return "竖直平面 Quad";
		if (i_path == "builtin:plane")
			return "水平地面 Plane";
		if (i_path == "builtin:sphere")
			return "球体 Sphere";
		if (i_path == "builtin:cylinder")
			return "圆柱 Cylinder";
		if (i_path == "builtin:cone")
			return "圆锥 Cone";
		if (i_path == "builtin:capsule")
			return "胶囊 Capsule";
		if (i_path == "builtin:torus")
			return "圆环 Torus";
		return i_path;
	}
	void EditorScene::ClearHistory()
	{
		// 历史和选择会持有物体，换场景前全部释放，避免选择指向旧场景。
		m_strokeMesh.reset();
		m_strokeBefore.clear();
		m_ownedTerrains.clear();
		m_selected = nullptr;
		m_owner = {};
		m_history.clear();
		m_historyPosition = 0;
		m_transformStarted = false;
	}
	void EditorScene::NewScene()
	{
		ClearHistory();
		m_data = MyEngine::SceneFileData();
		m_data.scene = std::make_unique<MyEngine::Scene>();
		MyEngine::ActorPtr sun(new MyEngine::Actor("Sun"));
		sun->AddComponent<Rendering::DirectionalLight>();
		m_data.scene->addActor(sun);
		AddModel("builtin:cube");
		m_history.clear();
		m_historyPosition = 0;
		m_dirty = false;
	}
	void EditorScene::Open(const std::filesystem::path &i_path)
	{
		// 先完整加载；缺贴图或文件损坏时抛错，当前正在编辑的场景还在。
		auto data = MyEngine::LoadSceneFile(i_path);
		// 天空图片也先实际解码；若图片损坏，不能先替换场景再在渲染初始化时失败。
		auto sky = std::make_shared<Rendering::TextureCube>();
		auto &faces = data.settings.skyFaces;
		sky->Init(faces[0].c_str(), faces[1].c_str(), faces[2].c_str(), faces[3].c_str(), faces[4].c_str(),
				  faces[5].c_str());
		if (!data.settings.ambientFaces[0].empty())
		{
			auto ambient = std::make_shared<Rendering::TextureCube>();
			const auto &a = data.settings.ambientFaces;
			ambient->Init(a[0].c_str(), a[1].c_str(), a[2].c_str(), a[3].c_str(), a[4].c_str(), a[5].c_str());
		}
		ClearHistory();
		m_data = std::move(data);
		m_dirty = false;
	}
	void EditorScene::Save(const std::filesystem::path &i_path, const Rendering::Camera &i_camera)
	{
		EndTerrainStroke();
		m_data.settings.cameraEye = i_camera.cameraPos;
		m_data.settings.cameraDirection = i_camera.viewVector;
		MyEngine::SaveSceneFile(*m_data.scene, i_path, m_data.settings);
		m_dirty = false;
	}
	MyEngine::ActorPtr EditorScene::AddModel(const std::string &i_path)
	{
		auto *meshes = Rendering::MeshManager::GetInstance();
		bool builtin = i_path.find("builtin:") == 0;
		auto mesh = builtin ? meshes->GetBuiltin(i_path) : meshes->Load(i_path, i_path);
		// 导入必须能在工程内独立使用。缺少 ../Textures 等依赖时，不能先显示成功再等保存报错。
		if (!mesh->m_importIssues.empty())
			throw std::runtime_error("模型资源不完整：" + mesh->m_importIssues.front() +
									 "。请把模型、材质和纹理放在同一资源目录后重试。");
		auto assetRoot = std::filesystem::canonical(Platform::GetAssetPath(""));
		for (const auto &dependency : mesh->m_importDependencies)
		{
			auto relative = std::filesystem::relative(std::filesystem::canonical(dependency), assetRoot);
			if (relative.empty() || relative.is_absolute() || *relative.begin() == L"..")
				throw std::runtime_error("模型仍引用工程外的文件：" + dependency.u8string() +
										 "。请整理相对资源引用后重新导入。");
		}
		std::string name = builtin ? GetModelDisplayName(i_path) : std::filesystem::u8path(i_path).stem().u8string();
		MyEngine::ActorPtr actor(new MyEngine::Actor(name));
		actor->AddComponent<Rendering::MeshRenderer>(mesh);
		// 大模型先缩到十米左右并放在地面上，拖进来即可看见，不必猜原文件的单位。
		if (!mesh->m_vertices.empty() && !mesh->m_terrain && i_path.find("builtin:") != 0)
		{
			glm::vec3 low(1e30f), high(-1e30f);
			for (const auto &vertex : mesh->m_vertices)
			{
				low = glm::min(low, vertex._position);
				high = glm::max(high, vertex._position);
			}
			auto size = high - low;
			float scale = 10.f / std::max({size.x, size.y, size.z, .001f});
			actor->m_scale = glm::vec3(scale);
			actor->m_position =
				Rendering::FromGLM(-glm::vec3((low.x + high.x) * .5f, low.y, (low.z + high.z) * .5f) * scale);
		}
		m_data.scene->addActor(actor);
		Select(actor, actor.operator->());
		Edit edit;
		edit.action = Action::Add;
		edit.owner = actor;
		edit.node = actor.operator->();
		Remember(edit);
		return actor;
	}
	MyEngine::ActorPtr EditorScene::AddTerrain()
	{
		auto mesh = Rendering::Mesh::CreateTerrain();
		MyEngine::ActorPtr actor(new MyEngine::Actor("Terrain"));
		actor->AddComponent<Rendering::MeshRenderer>(mesh);
		m_ownedTerrains.insert(mesh->GetHandle());
		m_data.scene->addActor(actor);
		Select(actor, actor.operator->());
		Edit edit;
		edit.action = Action::Add;
		edit.owner = actor;
		edit.node = actor.operator->();
		Remember(edit);
		return actor;
	}
	bool EditorScene::BeginTerrainStroke()
	{
		if (m_strokeMesh || !m_selected)
			return false;
		auto renderer = m_selected->GetComponent<Rendering::MeshRenderer>();
		if (!renderer || !renderer->GetMesh()->m_terrain)
			return false;
		auto mesh = renderer->GetMesh();
		// 首次编辑只复制当前节点的地形；共享模型和其他场景仍持有原高度。
		if (!m_ownedTerrains.count(mesh->GetHandle()))
		{
			mesh = mesh->CloneTerrain();
			renderer->SetMesh(mesh);
			m_ownedTerrains.insert(mesh->GetHandle());
		}
		m_strokeMesh = mesh;
		m_strokeBefore = mesh->m_terrain->heights;
		return true;
	}
	void EditorScene::EndTerrainStroke()
	{
		if (!m_strokeMesh)
			return;
		Edit edit;
		edit.action = Action::Terrain;
		edit.owner = m_owner;
		edit.node = m_selected;
		edit.terrain = m_strokeMesh;
		auto &after = m_strokeMesh->m_terrain->heights;
		// 一次拖笔只存真正变动的高度，不为每次鼠标移动压入整张高度图。
		for (unsigned i = 0; i < after.size(); ++i)
			if (after[i] != m_strokeBefore[i])
				edit.heights.push_back({i, m_strokeBefore[i], after[i]});
		m_strokeMesh.reset();
		m_strokeBefore.clear();
		if (!edit.heights.empty())
			Remember(edit);
	}
	void EditorScene::Select(const MyEngine::ActorPtr &i_owner, Rendering::SceneNode *i_node)
	{
		EndTerrainStroke();
		EndTransform();
		m_owner = i_owner;
		m_selected = i_node;
	}
	EditorScene::Transform EditorScene::ReadTransform(const Rendering::SceneNode &i_node)
	{
		return {i_node.m_position, i_node.m_orientation, i_node.m_scale};
	}
	void EditorScene::WriteTransform(Rendering::SceneNode &i_node, const Transform &i_transform)
	{
		i_node.m_position = i_transform.position;
		i_node.m_orientation = i_transform.rotation;
		i_node.m_scale = i_transform.scale;
	}
	void EditorScene::BeginTransform()
	{
		if (m_selected && !m_transformStarted)
		{
			m_before = ReadTransform(*m_selected);
			m_transformStarted = true;
		}
	}
	void EditorScene::EndTransform()
	{
		if (!m_transformStarted || !m_selected)
			return;
		m_transformStarted = false;
		auto after = ReadTransform(*m_selected);
		if (Rendering::ToGLM(after.position) == Rendering::ToGLM(m_before.position) &&
			after.rotation == m_before.rotation && after.scale == m_before.scale)
			return;
		Edit edit;
		edit.owner = m_owner;
		edit.node = m_selected;
		edit.before = m_before;
		edit.after = after;
		Remember(edit);
	}
	void EditorScene::DeleteSelected()
	{
		EndTerrainStroke();
		EndTransform();
		if (!m_owner || m_selected != m_owner.operator->())
			throw std::runtime_error("Please select the Actor itself to delete it (child nodes belong to it).");
		Edit edit;
		edit.action = Action::Remove;
		edit.owner = m_owner;
		edit.node = m_selected;
		if (m_data.scene->RemoveActor(m_owner))
			Remember(edit);
		m_selected = nullptr;
		m_owner = {};
	}
	void EditorScene::Remember(const Edit &i_edit)
	{
		// 撤销后再次操作，要丢掉旧的“重做分支”，这里不是 Git 分支。
		m_history.resize(m_historyPosition);
		m_history.push_back(i_edit);
		if (m_history.size() > 100)
			m_history.erase(m_history.begin());
		m_historyPosition = m_history.size();
		m_dirty = true;
	}
	void EditorScene::Apply(const Edit &i_edit, bool i_forward)
	{
		m_selected = nullptr;
		m_owner = {};
		if (i_edit.action == Action::Terrain)
		{
			for (const auto &height : i_edit.heights)
				i_edit.terrain->m_terrain->heights[height.index] = i_forward ? height.after : height.before;
			i_edit.terrain->m_terrain->dirty = true;
			Rendering::RebuildTerrain(*i_edit.terrain);
			m_selected = i_edit.node;
			m_owner = i_edit.owner;
		}
		else if (i_edit.action == Action::Transform)
			WriteTransform(*i_edit.node, i_forward ? i_edit.after : i_edit.before);
		else if ((i_edit.action == Action::Add) == i_forward)
			m_data.scene->addActor(i_edit.owner);
		else
			m_data.scene->RemoveActor(i_edit.owner);
		m_dirty = true;
	}
	void EditorScene::Undo()
	{
		EndTerrainStroke();
		EndTransform();
		if (m_historyPosition)
			Apply(m_history[--m_historyPosition], false);
	}
	void EditorScene::Redo()
	{
		EndTerrainStroke();
		EndTransform();
		if (m_historyPosition < m_history.size())
			Apply(m_history[m_historyPosition++], true);
	}
} // namespace Editor
