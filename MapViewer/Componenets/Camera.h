#pragma once
#include <glm.hpp>
#include <glew.h> // include GLEW and new version of GL on Windows
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"
#include <gtx/quaternion.hpp>

#include <assert.h>
#include <stdio.h> 
namespace Rendering
{
	class Camera
	{
	public:

		static glm::mat4x4 ModelToWorld_Matrix;
		static glm::mat4x4 WorldToView_Matrix;
		static glm::mat4x4 Projective_Matrix;


		glm::vec3 cameraPos;
		glm::vec3 viewVector;
		glm::vec3 upVector;


		glm::vec3 m_position;
		glm::vec3 m_scale;
		glm::quat m_orientation;

		void SetLookAt(glm::vec3& i_eye, glm::vec3& i_target, glm::vec3& i_up);
		void SetPerspective(float i_fov, float i_aspect, float i_zNear, float i_zFar);
		void RotateCamera(double Angle, double x, double y, double z);


		// Rotate around local Z
		void Roll(const float angle)
		{
			Rotate(glm::vec3(0, 0, 1), angle);
		}

		// Rotate around local X
		void Pitch(const float angle)
		{
			Rotate(glm::vec3(1, 0, 0), angle);
		}

		// Rotate around local Y
		void Yaw(const float angle)
		{
			Rotate(glm::vec3(0, 1, 0), angle);
		}

		void Rotate(const glm::vec3 &axis, const float angle)
		{
			using namespace glm;
			quat q;
			q = angleAxis(degrees(angle), axis);
			m_orientation = normalize( q * m_orientation);

		}


		glm::vec3 GetLeftDir();
		glm::vec3 GetForwardDir();

		void Update();

		static Camera* GetInstance()
		{
			assert(s_pInstance != nullptr);
			//printf("Camera not initialized!!");
			
			return s_pInstance;
		}

		Camera()
		{
			upVector = glm::vec3(0,1,0);
			m_position = glm::vec3(0.0f, 0.0f, 0.0f);
			m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
			m_orientation = glm::quat(glm::vec3(0, 0, 0));
			s_pInstance = this;
		}


	private:
		
		static Camera* s_pInstance;
	};

}

