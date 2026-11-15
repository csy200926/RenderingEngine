#pragma once
#include "Rendering/ISingleton.h"
#include <array>
#include <glm.hpp>
namespace MyEngine
{
	class InputManager : public Singleton<InputManager>
	{
	  public:
		void update()
		{
			m_previousKeyMap = m_keyMap;
			m_mouseDelta = glm::vec2(0);
		}
		void pressKey(unsigned i_key)
		{
			if (i_key < m_keyMap.size())
				m_keyMap[i_key] = true;
		}
		void releaseKey(unsigned i_key)
		{
			if (i_key < m_keyMap.size())
				m_keyMap[i_key] = false;
		}
		bool isKeyDown(unsigned i_key) const { return i_key < m_keyMap.size() && m_keyMap[i_key]; }
		bool isKeyPressed(unsigned i_key) const { return isKeyDown(i_key) && !m_previousKeyMap[i_key]; }
		void ReleaseAll()
		{
			m_keyMap.fill(false);
			m_previousKeyMap.fill(false);
			m_mouseDelta = glm::vec2(0);
		}
		void setMouseCoords(float i_x, float i_y)
		{
			glm::vec2 next(i_x, i_y);
			m_mouseDelta += next - m_mouseCoords;
			m_mouseCoords = next;
		}
		glm::vec2 getMouseCoords() const { return m_mouseCoords; }
		glm::vec2 GetMouseDelta() const { return m_mouseDelta; }

	  private:
		std::array<bool, 256> m_keyMap = {}, m_previousKeyMap = {};
		glm::vec2 m_mouseCoords = glm::vec2(0), m_mouseDelta = glm::vec2(0);
	};
} // namespace MyEngine
