#pragma once
#include "Engine/SceneFile.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include <vector>
#include <unordered_set>

namespace Editor
{
	// 资源保存仍用稳定的 builtin 标识；这里只给编辑器显示一个容易认的名称。
	std::string GetModelDisplayName(const std::string &i_path);
	// 这个类只处理“场景里有什么”和编辑历史，不知道窗口、DX 或 Vulkan。
	class EditorScene
	{
	  public:
		struct Transform
		{
			MyEngine::Vector3 position;
			glm::quat rotation = glm::quat(1, 0, 0, 0);
			glm::vec3 scale = glm::vec3(1);
		};
		void NewScene();
		void Open(const std::filesystem::path &i_path);
		void Save(const std::filesystem::path &i_path, const Rendering::Camera &i_camera);
		MyEngine::ActorPtr AddModel(const std::string &i_path);
		void Select(const MyEngine::ActorPtr &i_owner, Rendering::SceneNode *i_node);
		void DeleteSelected();
		MyEngine::ActorPtr AddTerrain();
		bool BeginTerrainStroke();
		void EndTerrainStroke();
		void BeginTransform();
		void EndTransform();
		void Undo();
		void Redo();
		void ClearHistory();
		MyEngine::Scene &GetScene() { return *m_data.scene; }
		const MyEngine::SceneFileSettings &GetSettings() const { return m_data.settings; }
		Rendering::SceneNode *GetSelected() const { return m_selected; }
		const MyEngine::ActorPtr &GetSelectedOwner() const { return m_owner; }
		bool IsDirty() const { return m_dirty; }
		void MarkDirty() { m_dirty = true; }
		static Transform ReadTransform(const Rendering::SceneNode &i_node);
		static void WriteTransform(Rendering::SceneNode &i_node, const Transform &i_transform);

	  private:
		enum class Action
		{
			Terrain,
			Transform,
			Add,
			Remove
		};
		struct HeightEdit
		{
			unsigned index;
			float before, after;
		};
		struct Edit
		{
			Action action = Action::Transform;
			MyEngine::ActorPtr owner;
			Rendering::SceneNode *node = nullptr;
			Transform before, after;
			Rendering::MeshPtr terrain;
			std::vector<HeightEdit> heights;
		};
		void Remember(const Edit &i_edit);
		void Apply(const Edit &i_edit, bool i_forward);
		MyEngine::SceneFileData m_data;
		MyEngine::ActorPtr m_owner;
		Rendering::SceneNode *m_selected = nullptr;
		std::vector<Edit> m_history;
		size_t m_historyPosition = 0;
		Transform m_before;
		Rendering::MeshPtr m_strokeMesh;
		std::vector<float> m_strokeBefore;
		std::unordered_set<uint64_t> m_ownedTerrains;
		bool m_transformStarted = false, m_dirty = false;
	};
} // namespace Editor
