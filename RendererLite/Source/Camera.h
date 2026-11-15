#pragma once
#include <glm.hpp>

namespace Rendering
{
	class Camera
	{
	public:
		Camera();

		glm::vec3 cameraPos;
		glm::vec3 viewVector;
		glm::vec3 upVector;

		glm::mat4 WorldToView_Matrix;
		glm::mat4 Projective_Matrix;

		void SetLookAt(const glm::vec3& i_eye, const glm::vec3& i_target, const glm::vec3& i_up);
		void SetPerspective(float i_fov, float i_aspect, float i_zNear, float i_zFar);
		void Update(float i_deltaTime, glm::vec2 i_mouseMove, bool i_keyboard);

	private:
		float m_yaw = 0;
		float m_pitch = 0;
	};
}
