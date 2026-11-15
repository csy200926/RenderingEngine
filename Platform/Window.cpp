#include "Window.h"
#include "Engine/InputManager.h"
#include "Log.h"
#include <windowsx.h>
#include <stdexcept>
namespace Platform
{
	Window::~Window()
	{
		if (m_window)
			DestroyWindow(m_window);
	}
	void Window::Initilize(const std::wstring &i_title, int i_width, int i_height)
	{
		SetProcessDPIAware();
		WNDCLASSW info = {};
		info.style = CS_OWNDC;
		info.lpfnWndProc = WindowProc;
		info.hInstance = GetModuleHandleW(nullptr);
		info.lpszClassName = L"MyEngineWindow";
		info.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		if (!RegisterClassW(&info) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			throw std::runtime_error("Window class registration failed");
		RECT rect = {0, 0, i_width, i_height};
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
		m_width = i_width;
		m_height = i_height;
		m_window =
			CreateWindowW(info.lpszClassName, i_title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
						  rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, info.hInstance, this);
		if (!m_window)
			throw std::runtime_error("Window creation failed");
		ShowWindow(m_window, SW_SHOW);
		UpdateWindow(m_window);
	}
	void Window::SetTitle(const std::wstring &i_title)
	{
		SetWindowTextW(m_window, i_title.c_str());
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
	LRESULT CALLBACK Window::WindowProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam)
	{
		auto *pWindow = reinterpret_cast<Window *>(GetWindowLongPtrW(i_window, GWLP_USERDATA));
		if (i_message == WM_NCCREATE)
		{
			pWindow = static_cast<Window *>(reinterpret_cast<CREATESTRUCTW *>(i_lParam)->lpCreateParams);
			SetWindowLongPtrW(i_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
		}
		if (!pWindow)
			return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
		auto *pInput = MyEngine::InputManager::GetInstance();
		switch (i_message)
		{
		case WM_CLOSE:
			pWindow->m_running = false;
			return 0;
		case WM_SIZE:
			pWindow->m_minimized = i_wParam == SIZE_MINIMIZED;
			pWindow->m_width = LOWORD(i_lParam);
			pWindow->m_height = HIWORD(i_lParam);
			pWindow->m_resized = true;
			return 0;
		case WM_KILLFOCUS:
			pInput->ReleaseAll();
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (i_wParam != VK_F4 || !(GetKeyState(VK_MENU) & 0x8000))
			{
				pInput->pressKey(static_cast<unsigned>(i_wParam));
				return 0;
			}
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			pInput->releaseKey(static_cast<unsigned>(i_wParam));
			return 0;
		case WM_LBUTTONDOWN:
			pInput->pressKey(VK_LBUTTON);
			return 0;
		case WM_LBUTTONUP:
			pInput->releaseKey(VK_LBUTTON);
			return 0;
		case WM_RBUTTONDOWN:
			pInput->pressKey(VK_RBUTTON);
			SetCapture(i_window);
			return 0;
		case WM_RBUTTONUP:
			pInput->releaseKey(VK_RBUTTON);
			ReleaseCapture();
			return 0;
		case WM_MOUSEMOVE:
			pInput->setMouseCoords(static_cast<float>(GET_X_LPARAM(i_lParam)),
								   static_cast<float>(GET_Y_LPARAM(i_lParam)));
			return 0;
		}
		return DefWindowProcW(i_window, i_message, i_wParam, i_lParam);
	}
} // namespace Platform
