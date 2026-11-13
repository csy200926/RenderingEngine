#pragma once
#include <glm.hpp>
#include <cstdint>
#include <vector>
#include "Camera.h"

namespace Rendering
{
	struct Vertex
	{
		glm::vec3 _position;
		glm::vec3 _normal;
		glm::vec2 _texCoords;
	};
	struct MeshData
	{
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};

	// 这几个参数一帧只更新一次。
	struct FrameConstants
	{
		glm::mat4 m_viewProjection = glm::mat4(1);
		glm::vec4 m_cameraPosition = glm::vec4(0);
		glm::vec4 m_sunDirection = glm::vec4(-.5f, -1, -.4f, 0);
	};
	struct DrawConstants
	{
		glm::mat4 m_model = glm::mat4(1);
		glm::vec4 m_color = glm::vec4(1);
	};
	struct DrawItem
	{
		uint32_t m_firstIndex = 0, m_indexCount = 0;
		DrawConstants m_constants;
	};
	struct FrameData
	{
		FrameConstants m_constants;
		std::vector<DrawItem> m_items;
	};

	static_assert(sizeof(Vertex) == 32, "Vertex layout changed");
	static_assert(sizeof(FrameConstants) == 96, "Frame constants must match Basic.hlsl");
	static_assert(sizeof(DrawConstants) == 80, "Draw constants must match Basic.hlsl");
	MeshData MakeDemoMesh();
	FrameData MakeDemoFrame(const Camera& i_camera, float i_time);
}
