#include "EditorApplication.h"
#include "Rendering/Terrain.h"
#include "Rendering/MeshRenderer.h"
#include <windowsx.h>
#include <gtc/matrix_transform.hpp>
#include <functional>
#include <algorithm>
#include <cmath>

namespace Editor
{
	static void Bounds(const Rendering::Mesh &i_mesh, glm::vec3 &o_low, glm::vec3 &o_high)
	{
		o_low = glm::vec3(1e30f);
		o_high = glm::vec3(-1e30f);
		for (auto &part : i_mesh.m_subMeshes)
		{
			o_low = glm::min(o_low, part.m_minimum);
			o_high = glm::max(o_high, part.m_maximum);
		}
	}
	void EditorApplication::FocusSelected()
	{
		auto node = m_scene.GetSelected();
		if (!node)
			return;
		auto matrix = node->GetFullTransform();
		glm::vec3 center(matrix * glm::vec4(0, 0, 0, 1));
		float radius = 2;
		if (auto renderer = node->GetComponent<Rendering::MeshRenderer>())
		{
			glm::vec3 low, high;
			Bounds(*renderer->GetMesh(), low, high);
			center = glm::vec3(matrix * glm::vec4((low + high) * .5f, 1));
			radius = glm::length(glm::vec3(matrix * glm::vec4(high - low, 0))) * .5f;
		}
		m_camera.cameraPos = center - m_camera.viewVector * std::max(2.f, radius * 2.8f);
		m_camera.Update();
		m_scene.MarkDirty();
	}
	void EditorApplication::UpdateCamera(float i_delta)
	{
		if (!m_looking || GetFocus() != m_view)
			return;
		float speed = (GetKeyState(VK_SHIFT) & 0x8000) ? 15.f : 4.f;
		glm::vec3 move(0);
		auto down = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
		if (down('W'))
			move += m_camera.viewVector;
		if (down('S'))
			move -= m_camera.viewVector;
		if (down('D'))
			move += m_camera.GetLeftDir();
		if (down('A'))
			move -= m_camera.GetLeftDir();
		if (down('E'))
			move.y += 1;
		if (down('Q'))
			move.y -= 1;
		if (glm::length(move) > 0)
		{
			m_camera.cameraPos += glm::normalize(move) * speed * i_delta;
			m_camera.Update();
			m_scene.MarkDirty();
		}
	}
	bool EditorApplication::Project(const glm::vec3 &i_point, POINT &o_point) const
	{
		// 世界位置先变到裁剪空间，再做透视除法，最后换成窗口左上角为原点的像素。
		auto clip = m_camera.Projective_Matrix * m_camera.WorldToView_Matrix * glm::vec4(i_point, 1);
		if (clip.w <= .001f)
			return false;
		auto point = glm::vec3(clip) / clip.w;
		if (!std::isfinite(point.x) || !std::isfinite(point.y))
			return false;
		o_point.x = static_cast<LONG>((point.x * .5f + .5f) * m_width);
		o_point.y = static_cast<LONG>((.5f - point.y * .5f) * m_height);
		return true;
	}
	void EditorApplication::GetGizmo(glm::vec3 &o_origin, std::array<glm::vec3, 3> &o_axes, float &o_size) const
	{
		auto node = m_scene.GetSelected();
		auto world = node->GetFullTransform();
		o_origin = glm::vec3(world[3]);
		o_size = std::max(.1f, glm::length(m_camera.cameraPos - o_origin) * .13f);
		for (int i = 0; i < 3; ++i)
		{
			o_axes[i] = glm::vec3(0);
			o_axes[i][i] = 1;
			if (m_local || m_tool == 2)
				o_axes[i] = glm::normalize(glm::vec3(world[i]));
		}
	}
	static float SegmentDistance(const POINT &i_a, const POINT &i_b, int i_x, int i_y)
	{
		glm::vec2 a{float(i_a.x), float(i_a.y)}, b{float(i_b.x), float(i_b.y)}, mouse{float(i_x), float(i_y)};
		auto direction = b - a;
		float length = glm::dot(direction, direction);
		float t = length < .01f ? 0.f : glm::clamp(glm::dot(mouse - a, direction) / length, .15f, 1.f);
		return glm::length(mouse - (a + direction * t));
	}
	void EditorApplication::BeginDrag(int i_x, int i_y)
	{
		if (m_propertiesDirty)
			ApplyProperties();
		m_axis = -1;
		if (m_tool == 3)
		{
			glm::vec3 hit;
			m_brushMouse = {i_x, i_y};
			if (TerrainHit(i_x, i_y, hit, true) && m_scene.BeginTerrainStroke())
			{
				m_terrainStroke = true;
				SetCapture(m_view);
				TerrainBrush(.016f);
			}
			return;
		}
		if (m_scene.GetSelected())
		{
			glm::vec3 origin;
			std::array<glm::vec3, 3> axes;
			float size;
			GetGizmo(origin, axes, size);
			POINT start, end;
			if (Project(origin, start))
			{
				float closest = 9;
				for (int i = 0; i < 3; ++i)
					if (Project(origin + axes[i] * size, end))
					{
						float distance = SegmentDistance(start, end, i_x, i_y);
						if (distance < closest)
						{
							closest = distance;
							m_axis = i;
						}
					}
			}
		}
		if (m_axis < 0)
		{
			Pick(i_x, i_y);
			return;
		}
		m_scene.BeginTransform();
		m_dragTransform = EditorScene::ReadTransform(*m_scene.GetSelected());
		m_dragStart = {i_x, i_y};
		m_dragging = true;
		SetCapture(m_view);
	}
	void EditorApplication::Drag(int i_x, int i_y)
	{
		m_brushMouse = {i_x, i_y};
		auto node = m_scene.GetSelected();
		if (!m_dragging || !node)
			return;
		// 每次从按下鼠标时的 Transform 重新计算，避免累计浮点误差和轴方向漂移。
		EditorScene::WriteTransform(*node, m_dragTransform);
		glm::vec3 origin;
		std::array<glm::vec3, 3> axes;
		float size;
		GetGizmo(origin, axes, size);
		POINT start, end;
		if (!Project(origin, start) || !Project(origin + axes[m_axis] * size, end))
			return;
		glm::vec2 direction(float(end.x - start.x), float(end.y - start.y));
		float pixels = glm::length(direction);
		if (pixels < 3)
			return;
		float amount = glm::dot(glm::vec2(i_x - m_dragStart.x, i_y - m_dragStart.y), direction / pixels) / pixels;
		if (m_tool == 0)
		{
			glm::vec3 displacement = axes[m_axis] * amount * size;
			// 属性里保存局部位置。父节点有旋转/缩放时，先把世界位移转回父空间。
			if (node->GetParent())
				displacement =
					glm::vec3(glm::inverse(node->GetParent()->GetFullTransform()) * glm::vec4(displacement, 0));
			node->m_position = Rendering::FromGLM(Rendering::ToGLM(m_dragTransform.position) + displacement);
		}
		else if (m_tool == 1)
		{
			float angle = amount * glm::radians(90.f);
			glm::vec3 axis(0);
			axis[m_axis] = 1;
			if (m_local)
				node->m_orientation = glm::normalize(m_dragTransform.rotation * glm::angleAxis(angle, axis));
			else
			{
				if (node->GetParent())
					axis = glm::normalize(glm::inverse(glm::mat3(node->GetParent()->GetFullTransform())) * axis);
				node->m_orientation = glm::normalize(glm::angleAxis(angle, axis) * m_dragTransform.rotation);
			}
		}
		else
			node->m_scale[m_axis] = m_dragTransform.scale[m_axis] * std::exp(glm::clamp(amount, -5.f, 5.f));
		RefreshProperties();
	}
	void EditorApplication::EndDrag()
	{
		if (m_terrainStroke)
		{
			m_terrainStroke = false;
			m_scene.EndTerrainStroke();
			ReleaseCapture();
			RefreshProperties();
		}
		if (!m_dragging)
			return;
		m_dragging = false;
		m_scene.EndTransform();
		ReleaseCapture();
		RefreshProperties();
	}
	bool EditorApplication::TerrainHit(int x, int y, glm::vec3 &hit, bool select)
	{
		auto inverse = glm::inverse(m_camera.Projective_Matrix * m_camera.WorldToView_Matrix);
		auto farPoint = inverse * glm::vec4(2.f * x / m_width - 1, 1 - 2.f * y / m_height, 1, 1);
		auto ray = glm::normalize(glm::vec3(farPoint) / farPoint.w - m_camera.cameraPos);
		float closest = 1e30f;
		Rendering::SceneNode *selected = nullptr;
		MyEngine::ActorPtr owner;
		std::function<void(Rendering::SceneNode &, const MyEngine::ActorPtr &)> visit;
		visit = [&](Rendering::SceneNode &node, const MyEngine::ActorPtr &actor)
		{
			if (select || &node == m_scene.GetSelected())
				if (auto renderer = node.GetComponent<Rendering::MeshRenderer>())
					if (renderer->GetMesh()->m_terrain)
					{
						auto local = glm::inverse(node.GetFullTransform());
						glm::vec3 point;
						if (Rendering::RaycastTerrain(*renderer->GetMesh(),
													  glm::vec3(local * glm::vec4(m_camera.cameraPos, 1)),
													  glm::vec3(local * glm::vec4(ray, 0)), point))
						{
							float distance = glm::length(glm::vec3(node.GetFullTransform() * glm::vec4(point, 1)) -
														 m_camera.cameraPos);
							if (distance < closest)
							{
								closest = distance;
								hit = point;
								selected = &node;
								owner = actor;
							}
						}
					}
			if (select)
				for (auto &child : node.GetChildren())
					visit(*child, actor);
		};
		if (select)
			for (auto &actor : m_scene.GetScene().GetActors())
				visit(*actor, actor);
		else if (m_scene.GetSelected())
			visit(*m_scene.GetSelected(), m_scene.GetSelectedOwner());
		if (select && selected)
		{
			m_scene.Select(owner, selected);
			SelectTreeItem();
		}
		return selected != nullptr;
	}
	void EditorApplication::TerrainBrush(float delta)
	{
		if (!m_terrainStroke || m_looking || m_tool != 3)
			return;
		glm::vec3 hit;
		if (!TerrainHit(m_brushMouse.x, m_brushMouse.y, hit, false))
			return;
		wchar_t text[80];
		GetWindowTextW(m_radius, text, 80);
		float radius = wcstof(text, nullptr);
		GetWindowTextW(m_strength, text, 80);
		float strength = wcstof(text, nullptr);
		if (!std::isfinite(radius) || !std::isfinite(strength) || radius <= 0 || strength <= 0)
			return;
		radius = glm::clamp(radius, .05f, 10000.f);
		strength = glm::clamp(strength, .001f, 10000.f);
		auto node = m_scene.GetSelected();
		auto matrix = node->GetFullTransform();
		float horizontal =
			std::max(.0001f, std::min(glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[2]))));
		float vertical = std::max(.0001f, glm::length(glm::vec3(matrix[1])));
		bool smooth = m_smoothTerrain || (GetKeyState(VK_CONTROL) & 0x8000);
		float amount = strength * delta / vertical;
		if (GetKeyState(VK_SHIFT) & 0x8000)
			amount = -amount;
		auto mesh = node->GetComponent<Rendering::MeshRenderer>()->GetMesh();
		if (Rendering::BrushTerrain(*mesh, hit, radius / horizontal, amount, smooth))
			m_scene.MarkDirty();
	}
	void EditorApplication::Pick(int i_x, int i_y)
	{
		// 从屏幕像素还原世界空间射线。各模型包围盒在自己的局部空间，便于处理旋转和缩放。
		auto inverse = glm::inverse(m_camera.Projective_Matrix * m_camera.WorldToView_Matrix);
		auto farPoint = inverse * glm::vec4(2.f * i_x / m_width - 1, 1 - 2.f * i_y / m_height, 1, 1);
		glm::vec3 direction = glm::normalize(glm::vec3(farPoint) / farPoint.w - m_camera.cameraPos);
		float closest = 1e30f;
		Rendering::SceneNode *selected = nullptr;
		MyEngine::ActorPtr owner;
		std::function<void(Rendering::SceneNode &, const MyEngine::ActorPtr &)> visit;
		visit = [&](Rendering::SceneNode &node, const MyEngine::ActorPtr &actor)
		{
			if (auto renderer = node.GetComponent<Rendering::MeshRenderer>())
			{
				auto local = glm::inverse(node.GetFullTransform());
				auto eye = glm::vec3(local * glm::vec4(m_camera.cameraPos, 1));
				auto ray = glm::vec3(local * glm::vec4(direction, 0));
				glm::vec3 low, high;
				Bounds(*renderer->GetMesh(), low, high);
				float nearT = 0, farT = closest;
				bool hit = true;
				for (int i = 0; i < 3; ++i)
				{
					if (std::abs(ray[i]) < 1e-8f)
					{
						if (eye[i] < low[i] || eye[i] > high[i])
							hit = false;
					}
					else
					{
						float a = (low[i] - eye[i]) / ray[i], b = (high[i] - eye[i]) / ray[i];
						nearT = std::max(nearT, std::min(a, b));
						farT = std::min(farT, std::max(a, b));
						if (nearT > farT)
							hit = false;
					}
				}
				if (hit && nearT < closest)
				{
					closest = nearT;
					selected = &node;
					owner = actor;
				}
			}
			for (auto &child : node.GetChildren())
				visit(*child, actor);
		};
		for (auto &actor : m_scene.GetScene().GetActors())
			visit(*actor, actor);
		m_scene.Select(owner, selected);
		SelectTreeItem();
	}
	void EditorApplication::DrawOverlay(HDC i_dc)
	{
		RECT rect = {0, 0, m_width, m_height};
		auto clear = CreateSolidBrush(RGB(255, 0, 255));
		FillRect(i_dc, &rect, clear);
		DeleteObject(clear);
		// 文字有深色底，避免 ClearType 的边缘与透明色混合成紫色毛边。
		SetBkMode(i_dc, OPAQUE);
		SetBkColor(i_dc, RGB(34, 38, 45));
		SelectObject(i_dc, m_font);
		SetTextColor(i_dc, RGB(225, 232, 240));
		const wchar_t *tool = m_tool == 3	? L"地形：左键抬高 / Shift降低 / Ctrl平滑；W/E/R 回到变换"
							  : m_tool == 0 ? L"移动：拖动彩色轴"
							  : m_tool == 1 ? L"旋转：沿彩色轴拖动"
											: L"缩放：拖动彩色轴";
		TextOutW(i_dc, 12, 12, tool, static_cast<int>(wcslen(tool)));
		auto node = m_scene.GetSelected();
		if (!node)
			return;
		if (auto renderer = node->GetComponent<Rendering::MeshRenderer>())
		{
			glm::vec3 low, high;
			Bounds(*renderer->GetMesh(), low, high);
			POINT corners[8];
			bool visible[8];
			for (int i = 0; i < 8; ++i)
				visible[i] = Project(
					glm::vec3(node->GetFullTransform() * glm::vec4((i & 1) ? high.x : low.x, (i & 2) ? high.y : low.y,
																   (i & 4) ? high.z : low.z, 1)),
					corners[i]);
			auto pen = CreatePen(PS_SOLID, 1, RGB(255, 190, 65));
			auto old = SelectObject(i_dc, pen);
			for (int i = 0; i < 8; ++i)
				for (int axis = 0; axis < 3; ++axis)
				{
					int next = i ^ (1 << axis);
					if (i < next && visible[i] && visible[next])
					{
						MoveToEx(i_dc, corners[i].x, corners[i].y, nullptr);
						LineTo(i_dc, corners[next].x, corners[next].y);
					}
				}
			SelectObject(i_dc, old);
			DeleteObject(pen);
		}
		if (m_tool == 3)
		{
			glm::vec3 hit;
			if (TerrainHit(m_brushMouse.x, m_brushMouse.y, hit, false))
			{
				auto renderer = node->GetComponent<Rendering::MeshRenderer>();
				auto &t = *renderer->GetMesh()->m_terrain;
				wchar_t text[80];
				GetWindowTextW(m_radius, text, 80);
				float radius = wcstof(text, nullptr);
				if (!std::isfinite(radius) || radius <= 0)
					return;
				auto matrix = node->GetFullTransform();
				radius =
					glm::clamp(radius, .05f, 10000.f) /
					std::max(.0001f, std::min(glm::length(glm::vec3(matrix[0])), glm::length(glm::vec3(matrix[2]))));
				auto pen = CreatePen(PS_SOLID, 2, RGB(255, 225, 90));
				auto old = SelectObject(i_dc, pen);
				bool previous = false;
				for (int i = 0; i <= 48; ++i)
				{
					float angle = i * 6.2831853f / 48;
					auto point = hit + glm::vec3(std::cos(angle) * radius, 0, std::sin(angle) * radius);
					if (point.x < 0 || point.z < 0 || point.x > t.sizeX || point.z > t.sizeZ)
					{
						previous = false;
						continue;
					}
					float gx = point.x / t.sizeX * (t.resX - 1), gz = point.z / t.sizeZ * (t.resZ - 1);
					unsigned x = std::min(unsigned(gx), t.resX - 2), z = std::min(unsigned(gz), t.resZ - 2);
					float u = gx - x, v = gz - z;
					float a = t.heights[z * t.resX + x], b = t.heights[z * t.resX + x + 1],
						  c = t.heights[(z + 1) * t.resX + x], d = t.heights[(z + 1) * t.resX + x + 1];
					point.y = (u + v <= 1) ? a + (b - a) * u + (c - a) * v : d + (c - d) * (1 - u) + (b - d) * (1 - v);
					POINT screen;
					if (Project(glm::vec3(matrix * glm::vec4(point, 1)), screen))
					{
						if (previous)
							LineTo(i_dc, screen.x, screen.y);
						else
							MoveToEx(i_dc, screen.x, screen.y, nullptr);
						previous = true;
					}
					else
						previous = false;
				}
				SelectObject(i_dc, old);
				DeleteObject(pen);
			}
			return;
		}
		glm::vec3 origin;
		std::array<glm::vec3, 3> axes;
		float size;
		GetGizmo(origin, axes, size);
		POINT start, end;
		COLORREF colors[] = {RGB(255, 90, 90), RGB(90, 245, 120), RGB(95, 160, 255)};
		if (!Project(origin, start))
			return;
		for (int i = 0; i < 3; ++i)
		{
			if (!Project(origin + axes[i] * size, end))
				continue;
			auto pen = CreatePen(PS_SOLID, 4, colors[i]);
			auto old = SelectObject(i_dc, pen);
			MoveToEx(i_dc, start.x, start.y, nullptr);
			LineTo(i_dc, end.x, end.y);
			Ellipse(i_dc, end.x - 4, end.y - 4, end.x + 4, end.y + 4);
			SetTextColor(i_dc, colors[i]);
			wchar_t label = L'X' + i;
			TextOutW(i_dc, end.x + 7, end.y - 7, &label, 1);
			SelectObject(i_dc, old);
			DeleteObject(pen);
		}
	}
	LRESULT CALLBACK EditorApplication::OverlayProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
	{
		auto self = reinterpret_cast<EditorApplication *>(GetWindowLongPtrW(i_window, GWLP_USERDATA));
		if (i_message == WM_NCCREATE)
		{
			self = static_cast<EditorApplication *>(reinterpret_cast<CREATESTRUCTW *>(i_lParam)->lpCreateParams);
			SetWindowLongPtrW(i_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		if (i_message == WM_NCHITTEST)
			return HTTRANSPARENT;
		if (i_message == WM_ERASEBKGND)
			return 1;
		if (self && i_message == WM_PAINT)
		{
			PAINTSTRUCT paint;
			auto dc = BeginPaint(i_window, &paint);
			self->DrawOverlay(dc);
			EndPaint(i_window, &paint);
			return 0;
		}
		return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
	}
	LRESULT CALLBACK EditorApplication::ViewProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
	{
		auto self = reinterpret_cast<EditorApplication *>(GetWindowLongPtrW(i_window, GWLP_USERDATA));
		if (i_message == WM_NCCREATE)
		{
			self = static_cast<EditorApplication *>(reinterpret_cast<CREATESTRUCTW *>(i_lParam)->lpCreateParams);
			SetWindowLongPtrW(i_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		if (!self || !self->m_ready)
			return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
		try
		{
			int x = GET_X_LPARAM(i_lParam), y = GET_Y_LPARAM(i_lParam);
			switch (i_message)
			{
			case WM_ERASEBKGND:
				return 1;
			case WM_LBUTTONDOWN:
				SetFocus(i_window);
				self->BeginDrag(x, y);
				return 0;
			case WM_LBUTTONUP:
				self->EndDrag();
				return 0;
			case WM_RBUTTONDOWN:
				SetFocus(i_window);
				self->m_looking = true;
				self->m_lastMouse = {x, y};
				SetCapture(i_window);
				return 0;
			case WM_RBUTTONUP:
				self->m_looking = false;
				ReleaseCapture();
				return 0;
			case WM_CAPTURECHANGED:
			case WM_KILLFOCUS:
				self->m_looking = false;
				self->EndDrag();
				return 0;
			case WM_MOUSEMOVE:
				if (self->m_looking)
				{
					self->m_yaw += (x - self->m_lastMouse.x) * .2f;
					self->m_pitch = glm::clamp(self->m_pitch - (y - self->m_lastMouse.y) * .2f, -89.f, 89.f);
					self->m_lastMouse = {x, y};
					float yaw = glm::radians(self->m_yaw), pitch = glm::radians(self->m_pitch);
					self->m_camera.viewVector =
						glm::vec3(std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch));
					self->m_camera.Update();
					self->m_scene.MarkDirty();
				}
				else
					self->Drag(x, y);
				return 0;
			}
		}
		catch (const std::exception &error)
		{
			self->ShowError(error);
		}
		return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
	}
} // namespace Editor
