#include "Camera.h"
#include <gtc/matrix_transform.hpp>
#include <stdexcept>
namespace Rendering
{
	void Camera::SetLookAt(const glm::vec3 &i_eye, const glm::vec3 &i_target, const glm::vec3 &i_up)
	{
		cameraPos = i_eye;
		viewVector = glm::normalize(i_target - i_eye);
		upVector = i_up;
		Update();
	}
	void Camera::SetPerspective(float i_fov, float i_aspect, float i_near, float i_far)
	{
		if (i_aspect <= 0 || i_near <= 0 || i_far <= i_near)
			throw std::invalid_argument("Invalid camera projection");
		Projective_Matrix = glm::perspective(glm::radians(i_fov), i_aspect, i_near, i_far);
	}
	void Camera::Update()
	{
		WorldToView_Matrix = glm::lookAt(cameraPos, cameraPos + viewVector, upVector);
	}
	glm::vec3 Camera::GetLeftDir() const
	{
		return glm::normalize(glm::cross(viewVector, upVector));
	}
} // namespace Rendering
