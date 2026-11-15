#include "Camera.h"
#include <windows.h>
#include <gtc/matrix_transform.hpp>
#include <cmath>

namespace Rendering
{
	static bool IsKeyDown(int i_key)
	{
		return (GetAsyncKeyState(i_key) & 0x8000) != 0;
	}

	Camera::Camera()
	{
		SetLookAt(glm::vec3(6, 4, 8), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));
		SetPerspective(55.0f, 1280.0f / 800.0f, 0.1f, 100.0f);
	}

	void Camera::SetLookAt(const glm::vec3& i_eye, const glm::vec3& i_target, const glm::vec3& i_up)
	{
		cameraPos = i_eye;
		viewVector = glm::normalize(i_target - i_eye);
		upVector = i_up;
		m_yaw = std::atan2(viewVector.x, -viewVector.z);
		m_pitch = std::asin(viewVector.y);

		WorldToView_Matrix = glm::lookAt(cameraPos, cameraPos + viewVector, upVector);
	}
	void Camera::SetPerspective(float i_fov, float i_aspect, float i_zNear, float i_zFar)
	{
		Projective_Matrix = glm::perspective(glm::radians(i_fov), i_aspect, i_zNear, i_zFar);
	}


	void Camera::Update(float i_deltaTime, glm::vec2 i_mouseMove, bool i_keyboard)
	{
		m_yaw += i_mouseMove.x * 0.003f;
		m_pitch -= i_mouseMove.y * 0.003f;
		m_pitch = glm::clamp(m_pitch, -1.5f, 1.5f);
		viewVector = glm::vec3(std::sin(m_yaw) * std::cos(m_pitch), std::sin(m_pitch),
			-std::cos(m_yaw) * std::cos(m_pitch));

		if(i_keyboard)
		{
			float moveSpeed = 4.0f;
			if(IsKeyDown(VK_SHIFT))
				moveSpeed = 15.0f;
			float step = moveSpeed * i_deltaTime;
			glm::vec3 right = glm::normalize(glm::cross(viewVector, upVector));

			if(IsKeyDown('W')) cameraPos += viewVector * step;
			if(IsKeyDown('S')) cameraPos -= viewVector * step;
			if(IsKeyDown('A')) cameraPos -= right * step;
			if(IsKeyDown('D')) cameraPos += right * step;

			if(IsKeyDown('Q')) cameraPos.y -= step;
			if(IsKeyDown('E')) cameraPos.y += step;
		}

		// viewVector 是方向，这里传给 lookAt 的仍然要是目标点。
		WorldToView_Matrix = glm::lookAt(cameraPos, cameraPos + viewVector, upVector);
	}
}
