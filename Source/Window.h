#pragma once
#include <windows.h>
#include <glm.hpp>

namespace Lite
{
	class Window
	{
	  public:
		~Window();
		void Initialize(unsigned i_width, unsigned i_height, bool i_hidden, const wchar_t *i_title);
		bool PumpEvents();
		glm::vec2 TakeMouseDelta();
		HWND m_handle = nullptr;
		unsigned m_width = 0, m_height = 0;
		bool m_resized = false, m_minimized = false;

	  private:
		bool m_running = true, m_rotating = false;
		POINT m_lastMouse = {};
		glm::vec2 m_mouseDelta = glm::vec2(0);
		static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);
	};
} // namespace Lite
