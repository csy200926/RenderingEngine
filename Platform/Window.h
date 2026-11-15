#pragma once
#include <windows.h>
#include <string>
namespace Platform
{
	class Window
	{
	  public:
		~Window();
		void Initilize(const std::wstring &i_title, int i_width, int i_height);
		bool PumpEvents();
		HWND GetHandle() const { return m_window; }
		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		bool IsMinimized() const { return m_minimized; }
		bool TakeResize()
		{
			bool result = m_resized;
			m_resized = false;
			return result;
		}
		void SetTitle(const std::wstring &i_title);

	  private:
		static LRESULT CALLBACK WindowProc(HWND i_window, UINT i_message, WPARAM i_wParam, LPARAM i_lParam);
		HWND m_window = nullptr;
		int m_width = 0, m_height = 0;
		bool m_running = true, m_resized = false, m_minimized = false;
	};
} // namespace Platform
