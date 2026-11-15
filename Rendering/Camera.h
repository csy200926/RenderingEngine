#pragma once
#include <glm.hpp>
namespace Rendering
{
	class Camera
	{
	  public:
		glm::vec3 cameraPos = glm::vec3(0, 2, 8), viewVector = glm::vec3(0, 0, -1), upVector = glm::vec3(0, 1, 0);
		glm::mat4 WorldToView_Matrix = glm::mat4(1), Projective_Matrix = glm::mat4(1);
		void SetLookAt(const glm::vec3 &i_eye, const glm::vec3 &i_target, const glm::vec3 &i_up);
		void SetPerspective(float i_fov, float i_aspect, float i_near, float i_far);
		void Update();
		glm::vec3 GetForwardDir() const { return viewVector; }
		glm::vec3 GetLeftDir() const;
	};
} // namespace Rendering
