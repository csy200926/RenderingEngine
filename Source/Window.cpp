#include "Window.h"
#include <windowsx.h>
#include <stdexcept>

namespace Lite
{
	Window::~Window()
	{
		if (m_handle)
			DestroyWindow(m_handle);
	}
	void Window::Initialize(unsigned i_width, unsigned i_height, bool i_hidden, const wchar_t *i_title)
	{
		SetProcessDPIAware();
		WNDCLASSW type = {};
		type.hInstance = GetModuleHandleW(nullptr);
		type.lpfnWndProc = Procedure;
		type.lpszClassName = L"RendererLiteWindow";
		type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		if (!RegisterClassW(&type) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			throw std::runtime_error("Cannot register window class");
		RECT rect = {0, 0, static_cast<LONG>(i_width), static_cast<LONG>(i_height)};
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
		m_handle =
			CreateWindowW(type.lpszClassName, i_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
						  rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, type.hInstance, this);
		if (!m_handle)
			throw std::runtime_error("Cannot create window");
		m_width = i_width;
		m_height = i_height;
		ShowWindow(m_handle, i_hidden ? SW_HIDE : SW_SHOW);
	}
	bool Window::PumpEvents()
	{
		MSG message;
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
		{
			if (message.message == WM_QUIT)
				m_running = false;
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
		return m_running;
	}
	glm::vec2 Window::TakeMouseDelta()
	{
		auto result = m_mouseDelta;
		m_mouseDelta = glm::vec2(0);
		return result;
	}
	LRESULT CALLBACK Window::Procedure(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
	{
		auto self = reinterpret_cast<Window *>(GetWindowLongPtrW(i_window, GWLP_USERDATA));
		if (i_message == WM_NCCREATE)
		{
			self = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(i_lParam)->lpCreateParams);
			SetWindowLongPtrW(i_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}
		if (!self)
			return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
		switch (i_message)
		{
		case WM_CLOSE:
			self->m_running = false;
			return 0;
		case WM_KEYDOWN:
			if (i_wParam == VK_ESCAPE)
				self->m_running = false;
			return 0;
		case WM_SIZE:
			// 窗口回调只记尺寸，等下一帧主循环等待 GPU 后重建交换链。
			self->m_width = LOWORD(i_lParam);
			self->m_height = HIWORD(i_lParam);
			self->m_minimized = i_wParam == SIZE_MINIMIZED;
			self->m_resized = true;
			return 0;
		case WM_RBUTTONDOWN:
			self->m_rotating = true;
			self->m_lastMouse = {GET_X_LPARAM(i_lParam), GET_Y_LPARAM(i_lParam)};
			SetCapture(i_window);
			return 0;
		case WM_MOUSEMOVE:
			if (self->m_rotating)
			{
				POINT now = {GET_X_LPARAM(i_lParam), GET_Y_LPARAM(i_lParam)};
				self->m_mouseDelta += glm::vec2(now.x - self->m_lastMouse.x, now.y - self->m_lastMouse.y);
				self->m_lastMouse = now;
			}
			return 0;
		case WM_RBUTTONUP:
			self->m_rotating = false;
			ReleaseCapture();
			return 0;
		case WM_CAPTURECHANGED:
			self->m_rotating = false;
			return 0;
		case WM_KILLFOCUS:
			self->m_rotating = false;
			self->m_mouseDelta = glm::vec2(0);
			if (GetCapture() == i_window)
				ReleaseCapture();
			return 0;
		}
		return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
	}
} // namespace Lite
