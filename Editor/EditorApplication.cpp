#include "EditorApplication.h"
#include "Rendering/Terrain.h"
#include <functional>
#include "Rendering/RenderingEngine.h"
#include "Rendering/MeshManager.h"
#include "Rendering/MaterialManager.h"
#include "Rendering/TextureManager.h"
#include "Engine/InputManager.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include <commdlg.h>
#include <shellapi.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cwctype>

namespace Editor
{
	enum Control
	{
		New = 100,
		Open,
		Save,
		SaveAs,
		Undo,
		Redo,
		AddCube,
		Import,
		AddResource,
		Delete,
		Apply,
		Move,
		Rotate,
		Scale,
		Space,
		Focus,
		AddTerrain,
		TerrainTool,
		SmoothTerrain,
		Tree = 200,
		Assets,
		Name,
		FirstNumber = 300
	};
	static HWND AddControl(HWND i_parent, const wchar_t *i_class, const wchar_t *i_text, DWORD i_style, int i_id,
						   HFONT i_font)
	{
		auto window =
			CreateWindowExW(0, i_class, i_text, WS_CHILD | WS_VISIBLE | i_style, 0, 0, 1, 1, i_parent,
							reinterpret_cast<HMENU>(static_cast<INT_PTR>(i_id)), GetModuleHandleW(nullptr), nullptr);
		if (!window)
			throw std::runtime_error("Cannot create editor control");
		SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(i_font), TRUE);
		return window;
	}
	EditorApplication::~EditorApplication()
	{
		m_ready = false;
		m_scene.ClearHistory();
		m_treeEntries.clear();
		Rendering::RenderingEngine::DestroyInstance();
		Rendering::MaterialManager::DestroyInstance();
		Rendering::MeshManager::DestroyInstance();
		Rendering::TextureManager::DestroyInstance();
		if (m_window)
			DestroyWindow(m_window);
		if (m_font)
			DeleteObject(m_font);
		if (m_background)
			DeleteObject(m_background);
		MyEngine::InputManager::DestroyInstance();
	}
	void EditorApplication::CreateWindows()
	{
		SetProcessDPIAware();
		INITCOMMONCONTROLSEX common = {sizeof(common), ICC_TREEVIEW_CLASSES};
		InitCommonControlsEx(&common);
		m_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		m_background = CreateSolidBrush(RGB(34, 38, 45));
		WNDCLASSW info = {};
		info.hInstance = GetModuleHandleW(nullptr);
		info.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		info.hbrBackground = m_background;
		info.lpszClassName = L"MyEngineEditor";
		info.lpfnWndProc = WindowProc;
		RegisterClassW(&info);
		info.lpszClassName = L"MyEngineEditorViewport";
		info.style = CS_OWNDC;
		info.lpfnWndProc = ViewProc;
		info.hbrBackground = nullptr;
		RegisterClassW(&info);
		info.lpszClassName = L"MyEngineEditorOverlay";
		info.style = 0;
		info.lpfnWndProc = OverlayProc;
		RegisterClassW(&info);
		m_window = CreateWindowW(L"MyEngineEditor", L"MyEngine 场景编辑器", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
								 CW_USEDEFAULT, CW_USEDEFAULT, 1450, 920, nullptr, nullptr, info.hInstance, this);
		if (!m_window)
			throw std::runtime_error("Cannot create editor window");
		m_view = CreateWindowW(L"MyEngineEditorViewport", L"Scene viewport", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0,
							   0, 1, 1, m_window, nullptr, info.hInstance, this);
		// 透明的编辑辅助层只画选中框和操作轴，不给渲染核心加编辑器回调。
		m_overlay =
			CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, L"MyEngineEditorOverlay", L"Gizmo",
							WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, m_window, nullptr, info.hInstance, this);
		if (!m_view || !m_overlay)
			throw std::runtime_error("Cannot create editor viewport / overlay");
		SetLayeredWindowAttributes(m_overlay, RGB(255, 0, 255), 255, LWA_COLORKEY);
		m_tree = AddControl(m_window, WC_TREEVIEWW, L"",
							TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, Tree, m_font);
		TreeView_SetBkColor(m_tree, RGB(26, 29, 35));
		TreeView_SetTextColor(m_tree, RGB(220, 225, 232));
		m_assets = AddControl(m_window, L"LISTBOX", L"", LBS_NOTIFY | WS_VSCROLL | WS_BORDER, Assets, m_font);
		SetWindowSubclass(m_assets, AssetProc, 1, reinterpret_cast<DWORD_PTR>(this));
		m_name = AddControl(m_window, L"EDIT", L"", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, Name, m_font);
		for (int i = 0; i < 9; ++i)
			m_numbers[i] =
				AddControl(m_window, L"EDIT", L"0", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, FirstNumber + i, m_font);
		const wchar_t *labels[] = {L"场景树",
								   L"物体属性",
								   L"名称",
								   L"位置  X / Y / Z",
								   L"旋转（度）X / Y / Z",
								   L"缩放  X / Y / Z",
								   L"资源：拖到视口或双击添加"};
		for (auto label : labels)
			m_labels.push_back(AddControl(m_window, L"STATIC", label, 0, 0, m_font));
		const wchar_t *buttons[] = {L"新建",	 L"打开",	  L"保存",	   L"另存为",	L"撤销",
									L"重做",	 L"添加方块", L"导入模型", L"添加资源", L"删除",
									L"应用属性", L"W 移动",	  L"E 旋转",   L"R 缩放",	L"世界坐标",
									L"F 聚焦",	 L"创建平地", L"地形笔刷", L"平滑：关"};
		for (int i = 0; i < 19; ++i)
			m_buttons.push_back(
				AddControl(m_window, L"BUTTON", buttons[i], BS_PUSHBUTTON | WS_TABSTOP, New + i, m_font));
		m_terrainLabel = AddControl(m_window, L"STATIC", L"地形 半径 / 强度（米/秒）", 0, 0, m_font);
		m_radius = AddControl(m_window, L"EDIT", L"5", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 400, m_font);
		m_strength = AddControl(m_window, L"EDIT", L"2", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 401, m_font);
		m_status = AddControl(m_window, L"STATIC", L"右键 + WASD 漫游；F 聚焦；Ctrl+S 保存；Ctrl+Z / Y 撤销 / 重做", 0,
							  0, m_font);
		Layout();
		ShowWindow(m_window, SW_SHOW);
		UpdateWindow(m_window);
	}
	void EditorApplication::Layout()
	{
		if (!m_view)
			return;
		RECT rect;
		GetClientRect(m_window, &rect);
		int width = rect.right, height = rect.bottom;
		int left = 220, right = std::max(left + 160, width - 290), bottom = std::max(180, height - 205);
		MoveWindow(m_view, left + 8, 52, std::max(1, right - left - 16), std::max(1, bottom - 60), TRUE);
		MoveWindow(m_overlay, left + 8, 52, std::max(1, right - left - 16), std::max(1, bottom - 60), TRUE);
		SetWindowPos(m_overlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		MoveWindow(m_tree, 8, 82, left - 12, std::max(1, bottom - 90), TRUE);
		if (m_labels.size() == 7)
		{
			MoveWindow(m_labels[0], 8, 55, 200, 24, TRUE);
			MoveWindow(m_labels[1], right + 8, 55, 260, 24, TRUE);
			MoveWindow(m_labels[2], right + 8, 88, 260, 24, TRUE);
			for (int row = 0; row < 3; ++row)
				MoveWindow(m_labels[row + 3], right + 8, 160 + row * 72, 260, 24, TRUE);
			MoveWindow(m_labels[6], 8, bottom + 6, 420, 24, TRUE);
		}
		MoveWindow(m_name, right + 8, 115, 264, 28, TRUE);
		for (int i = 0; i < 9; ++i)
			MoveWindow(m_numbers[i], right + 8 + (i % 3) * 89, 187 + (i / 3) * 72, 85, 28, TRUE);
		for (int i = 0; i < 8 && i < static_cast<int>(m_buttons.size()); ++i)
			MoveWindow(m_buttons[i], 8 + i * 93, 8, 88, 32, TRUE);
		if (m_buttons.size() == 19)
		{
			MoveWindow(m_buttons[8], width - 280, bottom + 40, 130, 30, TRUE);
			MoveWindow(m_buttons[9], right + 145, 400, 127, 30, TRUE);
			MoveWindow(m_buttons[10], right + 8, 400, 130, 30, TRUE);
			for (int i = 11; i <= 13; ++i)
				MoveWindow(m_buttons[i], right + 8 + (i - 11) * 89, 450, 85, 30, TRUE);
			MoveWindow(m_buttons[14], right + 8, 490, 130, 30, TRUE);
			MoveWindow(m_buttons[15], right + 145, 490, 127, 30, TRUE);
		}
		for (int i = 16; i < 19 && i < int(m_buttons.size()); ++i)
			MoveWindow(m_buttons[i], right + 8 + (i - 16) * 89, 530, 85, 30, TRUE);
		MoveWindow(m_terrainLabel, right + 8, 570, 264, 24, TRUE);
		MoveWindow(m_radius, right + 8, 600, 125, 28, TRUE);
		MoveWindow(m_strength, right + 145, 600, 127, 28, TRUE);
		MoveWindow(m_assets, 8, bottom + 36, std::max(1, width - 310), 130, TRUE);
		MoveWindow(m_status, 8, std::max(1, height - 28), std::max(1, width - 16), 24, TRUE);
		RECT view;
		GetClientRect(m_view, &view);
		m_width = std::max(1L, view.right);
		m_height = std::max(1L, view.bottom);
		if (m_ready)
		{
			Rendering::RenderingEngine::GetInstance()->Resize(m_width, m_height);
			auto &settings = m_scene.GetSettings();
			m_camera.SetPerspective(settings.cameraFov, float(m_width) / m_height, settings.cameraNear,
									settings.cameraFar);
		}
	}
	void EditorApplication::AddTreeNode(Rendering::SceneNode &i_node, const MyEngine::ActorPtr &i_owner,
										HTREEITEM i_parent)
	{
		auto name = Platform::ToWide(i_node.GetNodeName());
		TVINSERTSTRUCTW insert = {};
		insert.hParent = i_parent;
		insert.hInsertAfter = TVI_FIRST;
		insert.item.mask = TVIF_TEXT | TVIF_PARAM;
		insert.item.pszText = name.data();
		insert.item.lParam = static_cast<LPARAM>(m_treeEntries.size());
		auto item = TreeView_InsertItem(m_tree, &insert);
		m_treeEntries.push_back({&i_node, i_owner, item});
		for (auto child = i_node.GetChildren().rbegin(); child != i_node.GetChildren().rend(); ++child)
			AddTreeNode(**child, i_owner, item);
		if (!i_node.GetChildren().empty())
			TreeView_Expand(m_tree, item, TVE_EXPAND);
		if (&i_node == m_scene.GetSelected())
			TreeView_SelectItem(m_tree, item);
	}
	void EditorApplication::RefreshTree()
	{
		// 只有节点结构变化才重建树；选择/笔刷/属性更新只同步选中项。
		std::vector<Rendering::SceneNode *> nodes;
		std::function<void(Rendering::SceneNode &)> collect = [&](Rendering::SceneNode &node)
		{
			nodes.push_back(&node);
			for (auto child = node.GetChildren().rbegin(); child != node.GetChildren().rend(); ++child)
				collect(**child);
		};
		for (int i = m_scene.GetScene().GetActors().len - 1; i >= 0; --i)
			collect(*m_scene.GetScene().GetActors()[i]);
		bool same = nodes.size() == m_treeEntries.size();
		for (size_t i = 0; same && i < nodes.size(); ++i)
			same = nodes[i] == m_treeEntries[i].node;
		if (same)
		{
			SelectTreeItem();
			return;
		}
		m_refreshing = true;
		SendMessageW(m_tree, WM_SETREDRAW, FALSE, 0);
		TreeView_DeleteAllItems(m_tree);
		m_treeEntries.clear();
		for (int i = m_scene.GetScene().GetActors().len - 1; i >= 0; --i)
			AddTreeNode(*m_scene.GetScene().GetActors()[i], m_scene.GetScene().GetActors()[i], TVI_ROOT);
		SendMessageW(m_tree, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(m_tree, nullptr, TRUE);
		m_refreshing = false;
		RefreshProperties();
	}
	void EditorApplication::SelectTreeItem()
	{
		m_refreshing = true;
		HTREEITEM selected = nullptr;
		for (auto &entry : m_treeEntries)
		{
			if (entry.node != m_scene.GetSelected())
				continue;
			selected = entry.item;
			auto name = Platform::ToWide(entry.node->GetNodeName());
			TVITEMW item = {};
			item.mask = TVIF_TEXT;
			item.hItem = entry.item;
			item.pszText = name.data();
			TreeView_SetItem(m_tree, &item);
		}
		TreeView_SelectItem(m_tree, selected);
		m_refreshing = false;
		RefreshProperties();
	}
	void EditorApplication::RefreshProperties()
	{
		m_refreshing = true;
		auto node = m_scene.GetSelected();
		SetWindowTextW(m_name, node ? Platform::ToWide(node->GetNodeName()).c_str() : L"");
		glm::vec3 values[3] = {glm::vec3(0), glm::vec3(0), glm::vec3(1)};
		if (node)
		{
			values[0] = Rendering::ToGLM(node->m_position);
			values[1] = glm::degrees(glm::eulerAngles(node->m_orientation));
			values[2] = node->m_scale;
		}
		for (int i = 0; i < 9; ++i)
		{
			std::wostringstream number;
			number << std::fixed << std::setprecision(4) << values[i / 3][i % 3];
			SetWindowTextW(m_numbers[i], number.str().c_str());
			EnableWindow(m_numbers[i], node != nullptr);
		}
		EnableWindow(m_name, node != nullptr);
		m_propertiesDirty = false;
		m_refreshing = false;
	}
	void EditorApplication::ApplyProperties()
	{
		auto node = m_scene.GetSelected();
		if (!node)
			return;
		float values[9];
		// 先读完并检查九个数，再一起应用，避免输入错误时只改了一半。
		for (int i = 0; i < 9; ++i)
		{
			wchar_t text[128];
			GetWindowTextW(m_numbers[i], text, 128);
			wchar_t *end = nullptr;
			values[i] = std::wcstof(text, &end);
			while (end && std::iswspace(*end))
				++end;
			if (end == text || (end && *end) || !std::isfinite(values[i]) || std::abs(values[i]) > 1e7f)
				throw std::runtime_error("Transform must contain finite numbers.");
			if (i >= 6 && std::abs(values[i]) < .0001f)
				throw std::runtime_error("Scale cannot be zero.");
		}
		wchar_t name[256];
		GetWindowTextW(m_name, name, 256);
		m_scene.BeginTransform();
		node->m_position = MyEngine::Vector3(values[0], values[1], values[2]);
		// 面板给人看的是角度，GLM 的四元数构造函数接收弧度。
		node->m_orientation = glm::quat(glm::radians(glm::vec3(values[3], values[4], values[5])));
		node->m_scale = glm::vec3(values[6], values[7], values[8]);
		m_scene.EndTransform();
		if (node->GetNodeName() != Platform::ToUTF8(name))
		{
			node->SetNodeName(Platform::ToUTF8(name));
			m_scene.MarkDirty();
		}
		RefreshTree();
	}
	void EditorApplication::RefreshAssets()
	{
		m_assetPaths = Rendering::Mesh::GetBuiltinNames();
		size_t builtinCount = m_assetPaths.size();
		auto root = Platform::GetAssetPath("");
		for (const auto &file : std::filesystem::recursive_directory_iterator(root))
		{
			if (!file.is_regular_file())
				continue;
			auto ext = file.path().extension().wstring();
			for (auto &ch : ext)
				ch = static_cast<wchar_t>(std::towlower(ch));
			if (ext == L".obj" || ext == L".fbx" || ext == L".terrain")
				m_assetPaths.push_back(std::filesystem::relative(file.path(), root).u8string());
		}
		std::sort(m_assetPaths.begin() + builtinCount, m_assetPaths.end());
		SendMessageW(m_assets, LB_RESETCONTENT, 0, 0);
		for (const auto &path : m_assetPaths)
			SendMessageW(m_assets, LB_ADDSTRING, 0,
						 reinterpret_cast<LPARAM>(Platform::ToWide(GetModelDisplayName(path)).c_str()));
	}
	void EditorApplication::AddAsset(int i_index)
	{
		if (m_propertiesDirty)
			ApplyProperties();
		if (i_index < 0 || i_index >= static_cast<int>(m_assetPaths.size()))
			return;
		m_scene.AddModel(m_assetPaths[i_index]);
		RefreshTree();
		FocusSelected();
		SetFocus(m_view);
	}
	static std::filesystem::path ChooseFile(HWND i_owner, bool i_save, const wchar_t *i_filter,
											const wchar_t *i_extension)
	{
		wchar_t buffer[32768] = {};
		OPENFILENAMEW dialog = {};
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = i_owner;
		dialog.lpstrFilter = i_filter;
		dialog.lpstrFile = buffer;
		dialog.nMaxFile = 32768;
		dialog.lpstrDefExt = i_extension;
		dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (i_save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
		if (!(i_save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog)))
			return {};
		return buffer;
	}
	void EditorApplication::ImportModel()
	{
		auto file = ChooseFile(m_window, false, L"模型 OBJ / FBX\0*.obj;*.fbx\0\0", L"obj");
		if (file.empty())
			return;
		auto root = std::filesystem::weakly_canonical(Platform::GetAssetPath(""));
		auto relative = std::filesystem::relative(file, root);
		if (relative.empty() || *relative.begin() == L"..")
		{
			// OBJ 常在旁边引用 MTL 和纹理。保留原目录布局，不能只复制模型本身。
			auto source = file.parent_path();
			auto destination = root / L"Models" / L"Imported" / file.stem();
			for (int i = 1; std::filesystem::exists(destination); ++i)
				destination = root / L"Models" / L"Imported" / (file.stem().wstring() + L"_" + std::to_wstring(i));
			std::vector<std::filesystem::path> files;
			uintmax_t size = 0;
			for (auto it = std::filesystem::recursive_directory_iterator(source);
				 it != std::filesystem::recursive_directory_iterator(); ++it)
			{
				if (it.depth() > 8 || it->is_symlink())
				{
					if (it->is_directory())
						it.disable_recursion_pending();
					continue;
				}
				if (!it->is_regular_file())
					continue;
				auto ext = it->path().extension().wstring();
				for (auto &ch : ext)
					ch = static_cast<wchar_t>(std::towlower(ch));
				if (ext != L".obj" && ext != L".fbx" && ext != L".mtl" && ext != L".png" && ext != L".jpg" &&
					ext != L".jpeg" && ext != L".tga" && ext != L".bmp" && ext != L".dds")
					continue;
				files.push_back(it->path());
				size += it->file_size();
				if (files.size() > 2048 || size > 512ull * 1024 * 1024)
					throw std::runtime_error("Import folder is too large. Put this model and its textures in a smaller "
											 "folder first (limit 512 MiB / 2048 files).");
			}
			for (const auto &entry : files)
			{
				auto target = destination / std::filesystem::relative(entry, source);
				std::filesystem::create_directories(target.parent_path());
				std::filesystem::copy_file(entry, target);
			}
			file = destination / file.filename();
		}
		m_scene.AddModel(std::filesystem::relative(file, root).u8string());
		RefreshAssets();
		RefreshTree();
		FocusSelected();
		Platform::Log("Editor imported model: " + file.u8string() +
					  "; missing texture warnings, if any, are in this log.");
	}
	bool EditorApplication::SaveScene(bool i_saveAs)
	{
		if (m_propertiesDirty)
			ApplyProperties();
		auto path = m_scenePath;
		if (path.empty() || i_saveAs)
			path = ChooseFile(m_window, true, L"Lua 场景\0*.lua\0\0", L"lua");
		if (path.empty())
			return false;
		m_scene.Save(path, m_camera);
		m_scenePath = path;
		Platform::Log("Editor saved scene: " + path.u8string());
		return true;
	}
	bool EditorApplication::CanDiscard()
	{
		if (!m_scene.IsDirty() && !m_propertiesDirty)
			return true;
		int answer =
			MessageBoxW(m_window, L"场景有未保存的修改，是否先保存？", L"场景编辑器", MB_YESNOCANCEL | MB_ICONQUESTION);
		return answer == IDNO || (answer == IDYES && SaveScene());
	}
	void EditorApplication::OpenScene()
	{
		if (!CanDiscard())
			return;
		auto path = ChooseFile(m_window, false, L"Lua 场景\0*.lua\0\0", L"lua");
		if (path.empty())
			return;
		m_scene.Open(path);
		m_treeEntries.clear();
		m_scenePath = path;
		ApplyCamera();
		ApplyEnvironment();
		RefreshTree();
	}
	void EditorApplication::ApplyEnvironment()
	{
		auto &settings = m_scene.GetSettings();
		auto sky = std::make_shared<Rendering::TextureCube>();
		sky->Init(settings.skyFaces[0].c_str(), settings.skyFaces[1].c_str(), settings.skyFaces[2].c_str(),
				  settings.skyFaces[3].c_str(), settings.skyFaces[4].c_str(), settings.skyFaces[5].c_str());
		auto engine = Rendering::RenderingEngine::GetInstance();
		Rendering::TextureCubePtr ambient;
		if (!settings.ambientFaces[0].empty())
		{
			ambient = std::make_shared<Rendering::TextureCube>();
			const auto &a = settings.ambientFaces;
			ambient->Init(a[0].c_str(), a[1].c_str(), a[2].c_str(), a[3].c_str(), a[4].c_str(), a[5].c_str());
		}
		engine->SetEnvironment(sky, ambient);
		engine->SetDrawSky(settings.drawSky);
	}
	void EditorApplication::ApplyCamera()
	{
		auto &settings = m_scene.GetSettings();
		m_camera.SetLookAt(settings.cameraEye, settings.cameraEye + settings.cameraDirection, glm::vec3(0, 1, 0));
		m_camera.SetPerspective(settings.cameraFov, float(m_width) / m_height, settings.cameraNear, settings.cameraFar);
		m_yaw = glm::degrees(std::atan2(m_camera.viewVector.z, m_camera.viewVector.x));
		m_pitch = glm::degrees(std::asin(glm::clamp(m_camera.viewVector.y, -.999f, .999f)));
	}
	void EditorApplication::Command(unsigned i_command)
	{
		// 保存、切工具、添加或删除前先收下属性输入，避免忘点“应用”而丢掉刚输入的数值。
		if (m_propertiesDirty && i_command != New && i_command != Open && i_command != Apply)
			ApplyProperties();
		EndDrag();
		switch (i_command)
		{
		case New:
			if (CanDiscard())
			{
				m_scene.NewScene();
				m_scenePath.clear();
				ApplyCamera();
				ApplyEnvironment();
				RefreshTree();
			}
			break;
		case Open:
			OpenScene();
			break;
		case Save:
			SaveScene();
			break;
		case SaveAs:
			SaveScene(true);
			break;
		case Undo:
			m_scene.Undo();
			RefreshTree();
			break;
		case Redo:
			m_scene.Redo();
			RefreshTree();
			break;
		case AddCube:
			m_scene.AddModel("builtin:cube");
			RefreshTree();
			FocusSelected();
			break;
		case Import:
			ImportModel();
			break;
		case AddResource:
			AddAsset(static_cast<int>(SendMessageW(m_assets, LB_GETCURSEL, 0, 0)));
			break;
		case Delete:
			m_scene.DeleteSelected();
			RefreshTree();
			break;
		case Apply:
			ApplyProperties();
			break;
		case AddTerrain:
			m_scene.AddTerrain();
			m_tool = 3;
			RefreshTree();
			FocusSelected();
			break;
		case TerrainTool:
			m_tool = 3;
			break;
		case SmoothTerrain:
			m_smoothTerrain = !m_smoothTerrain;
			m_tool = 3;
			SetWindowTextW(m_buttons[18], m_smoothTerrain ? L"平滑：开" : L"平滑：关");
			break;
		case Move:
			m_tool = 0;
			break;
		case Rotate:
			m_tool = 1;
			break;
		case Scale:
			m_tool = 2;
			break;
		case Space:
			m_local = !m_local;
			SetWindowTextW(m_buttons[14], m_local ? L"局部坐标" : L"世界坐标");
			break;
		case Focus:
			FocusSelected();
			break;
		}
	}
	void EditorApplication::ShowError(const std::exception &i_error)
	{
		Platform::Log(std::string("Editor: ") + i_error.what());
		MessageBoxW(m_window, Platform::ToWide(i_error.what()).c_str(), L"操作未完成", MB_OK | MB_ICONERROR);
	}
	int EditorApplication::Run(const std::string &i_renderer, int i_frames, const std::filesystem::path &i_capture,
							   const std::filesystem::path &i_sceneFile)
	{
		m_renderer = i_renderer;
		auto mode = Rendering::RenderMode::OpenGL;
		if (i_renderer == "dx11")
			mode = Rendering::RenderMode::DX11;
		else if (i_renderer == "dx12")
			mode = Rendering::RenderMode::DX12;
		else if (i_renderer == "vulkan")
			mode = Rendering::RenderMode::Vulkan;
		else if (i_renderer != "opengl")
			throw std::invalid_argument("Editor renderer is not integrated yet: " + i_renderer);
		CreateWindows();
		m_scene.NewScene();
		if (!i_sceneFile.empty())
		{
			m_scene.Open(i_sceneFile);
			m_scenePath = i_sceneFile;
		}
		auto engine = Rendering::RenderingEngine::GetInstance();
		engine->Initilize(mode, m_view, m_width, m_height, 4);
		engine->SetProfiling(true);
		ApplyCamera();
		ApplyEnvironment();
		RefreshAssets();
		RefreshTree();
		m_ready = true;
		auto previous = std::chrono::steady_clock::now();
		int frame = 0;
		Platform::Log("Editor running: " + i_renderer);
		while (m_running)
		{
			MSG message;
			while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
					m_running = false;
				// 快捷键只在视口使用。属性输入框里的 W/E/R、Delete、Ctrl+Z 保留正常编辑行为。
				if (message.message == WM_KEYDOWN && GetFocus() == m_view && !m_looking)
				{
					bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
					unsigned command = control && message.wParam == 'S'	  ? Save
									   : control && message.wParam == 'Z' ? Undo
									   : control && message.wParam == 'Y' ? Redo
									   : message.wParam == 'W'			  ? Move
									   : message.wParam == 'E'			  ? Rotate
									   : message.wParam == 'R'			  ? Scale
									   : message.wParam == 'F'			  ? Focus
									   : message.wParam == VK_DELETE	  ? Delete
																		  : 0;
					if (command)
					{
						try
						{
							Command(command);
						}
						catch (const std::exception &error)
						{
							ShowError(error);
						}
						continue;
					}
				}
				TranslateMessage(&message);
				DispatchMessageW(&message);
			}
			if (!m_running)
				break;
			auto now = std::chrono::steady_clock::now();
			float delta = std::min(.05f, std::chrono::duration<float>(now - previous).count());
			previous = now;
			if (IsIconic(m_window))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				continue;
			}
			UpdateCamera(delta);
			TerrainBrush(delta);
			// 编辑模式不推进游戏/碰撞模拟；同一 Scene 只被收集、绘制。
			engine->Render(m_scene.GetScene(), m_camera,
						   (i_frames && frame + 1 == i_frames) ? i_capture : std::filesystem::path(), true);
			InvalidateRect(m_overlay, nullptr, FALSE);
			UpdateWindow(m_overlay);
			if ((frame % 30) == 0)
			{
				std::wostringstream title;
				title << L"MyEngine 场景编辑器 | " << Platform::ToWide(m_renderer) << L" | "
					  << (m_scenePath.empty() ? L"未命名场景" : m_scenePath.filename().wstring())
					  << (m_scene.IsDirty() ? L" *" : L"") << L" | GPU " << std::fixed << std::setprecision(2)
					  << engine->GetGPUTimeMilliseconds() << L" ms";
				SetWindowTextW(m_window, title.str().c_str());
			}
			if (i_frames && ++frame >= i_frames)
				m_running = false;
			else if (!i_frames)
				++frame;
		}
		return 0;
	}
	LRESULT CALLBACK EditorApplication::WindowProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
	{
		auto self = reinterpret_cast<EditorApplication *>(GetWindowLongPtrW(i_window, GWLP_USERDATA));
		if (i_message == WM_NCCREATE)
		{
			self = static_cast<EditorApplication *>(reinterpret_cast<CREATESTRUCTW *>(i_lParam)->lpCreateParams);
			SetWindowLongPtrW(i_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
			self->m_window = i_window;
		}
		if (!self)
			return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
		try
		{
			switch (i_message)
			{
			case WM_SIZE:
				self->Layout();
				return 0;
			case WM_CLOSE:
				if (self->CanDiscard())
					self->m_running = false;
				return 0;
			case WM_GETMINMAXINFO:
				reinterpret_cast<MINMAXINFO *>(i_lParam)->ptMinTrackSize = {1050, 880};
				return 0;
			case WM_COMMAND:
				if (!self->m_refreshing && HIWORD(i_wParam) == EN_CHANGE &&
					(LOWORD(i_wParam) == Name ||
					 (LOWORD(i_wParam) >= FirstNumber && LOWORD(i_wParam) < FirstNumber + 9)))
				{
					self->m_propertiesDirty = true;
					return 0;
				}
				if (LOWORD(i_wParam) == Assets && HIWORD(i_wParam) == LBN_DBLCLK)
					self->AddAsset(static_cast<int>(SendMessageW(self->m_assets, LB_GETCURSEL, 0, 0)));
				else if (HIWORD(i_wParam) == BN_CLICKED)
					self->Command(LOWORD(i_wParam));
				return 0;
			case WM_NOTIFY:
				if (!self->m_refreshing && reinterpret_cast<NMHDR *>(i_lParam)->code == TVN_SELCHANGEDW)
				{
					auto change = reinterpret_cast<NMTREEVIEWW *>(i_lParam);
					auto index = static_cast<size_t>(change->itemNew.lParam);
					if (index < self->m_treeEntries.size())
					{
						auto entry = self->m_treeEntries[index];
						if (self->m_propertiesDirty)
							self->ApplyProperties();
						self->EndDrag();
						self->m_scene.Select(entry.owner, entry.node);
						self->RefreshProperties();
					}
				}
				return 0;
			case WM_CTLCOLORSTATIC:
				SetTextColor(reinterpret_cast<HDC>(i_wParam), RGB(220, 225, 232));
				SetBkColor(reinterpret_cast<HDC>(i_wParam), RGB(34, 38, 45));
				return reinterpret_cast<LRESULT>(self->m_background);
			}
		}
		catch (const std::exception &error)
		{
			self->ShowError(error);
		}
		return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
	}
	LRESULT CALLBACK EditorApplication::AssetProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam,
												  UINT_PTR, DWORD_PTR i_data)
	{
		auto self = reinterpret_cast<EditorApplication *>(i_data);
		if (i_message == WM_LBUTTONDOWN)
		{
			auto index = SendMessageW(i_window, LB_ITEMFROMPOINT, 0, i_lParam);
			self->m_dragAsset = HIWORD(index) ? -1 : LOWORD(index);
			if (self->m_dragAsset >= 0)
			{
				SendMessageW(i_window, LB_SETCURSEL, self->m_dragAsset, 0);
				SetCapture(i_window);
				return 0;
			}
		}
		if (i_message == WM_LBUTTONUP && self->m_dragAsset >= 0)
		{
			int index = self->m_dragAsset;
			self->m_dragAsset = -1;
			ReleaseCapture();
			POINT cursor;
			GetCursorPos(&cursor);
			RECT rect;
			GetWindowRect(self->m_view, &rect);
			if (PtInRect(&rect, cursor))
				try
				{
					self->AddAsset(index);
				}
				catch (const std::exception &error)
				{
					self->ShowError(error);
				}
			return 0;
		}
		return DefSubclassProc(i_window, i_message, i_wParam, i_lParam);
	}
} // namespace Editor
