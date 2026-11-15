#pragma once
#include "EditorScene.h"
#include <windows.h>
#include <commctrl.h>
#include <array>

namespace Editor
{
	class EditorApplication
	{
	  public:
		~EditorApplication();
		int Run(const std::string &i_renderer, int i_frames, const std::filesystem::path &i_capture,
				const std::filesystem::path &i_sceneFile);

	  private:
		static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK ViewProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK OverlayProc(HWND, UINT, WPARAM, LPARAM);
		static LRESULT CALLBACK AssetProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
		void CreateWindows();
		void Layout();
		void RefreshTree();
		void AddTreeNode(Rendering::SceneNode &, const MyEngine::ActorPtr &, HTREEITEM);
		void RefreshProperties();
		void ApplyProperties();
		void RefreshAssets();
		void Command(unsigned i_command);
		void AddAsset(int i_index);
		void ImportModel();
		void OpenScene();
		bool SaveScene(bool i_saveAs = false);
		bool CanDiscard();
		void ApplyEnvironment();
		void ApplyCamera();
		void FocusSelected();
		void UpdateCamera(float i_delta);
		void Pick(int i_x, int i_y);
		void SelectTreeItem();
		bool TerrainHit(int i_x, int i_y, glm::vec3 &o_hit, bool i_select);
		void TerrainBrush(float i_delta);
		void BeginDrag(int i_x, int i_y);
		void Drag(int i_x, int i_y);
		void EndDrag();
		bool Project(const glm::vec3 &, POINT &) const;
		void DrawOverlay(HDC);
		void GetGizmo(glm::vec3 &i_origin, std::array<glm::vec3, 3> &i_axes, float &i_size) const;
		void ShowError(const std::exception &);
		HWND m_window = nullptr, m_view = nullptr, m_overlay = nullptr, m_tree = nullptr;
		HWND m_radius = nullptr, m_strength = nullptr, m_terrainLabel = nullptr;
		bool m_terrainStroke = false, m_smoothTerrain = false;
		POINT m_brushMouse = {};
		HWND m_assets = nullptr, m_name = nullptr, m_status = nullptr;
		std::array<HWND, 9> m_numbers = {};
		std::vector<HWND> m_labels, m_buttons;
		struct TreeEntry
		{
			Rendering::SceneNode *node;
			MyEngine::ActorPtr owner;
			HTREEITEM item;
		};
		std::vector<TreeEntry> m_treeEntries;
		std::vector<std::string> m_assetPaths;
		EditorScene m_scene;
		Rendering::Camera m_camera;
		std::filesystem::path m_scenePath;
		std::string m_renderer;
		bool m_running = true, m_refreshing = false, m_local = false, m_looking = false;
		bool m_dragging = false, m_ready = false;
		bool m_propertiesDirty = false;
		int m_width = 1, m_height = 1, m_tool = 0, m_axis = -1, m_dragAsset = -1;
		POINT m_lastMouse = {}, m_dragStart = {};
		EditorScene::Transform m_dragTransform;
		float m_yaw = -90, m_pitch = 0;
		HFONT m_font = nullptr;
		HBRUSH m_background = nullptr;
	};
} // namespace Editor
