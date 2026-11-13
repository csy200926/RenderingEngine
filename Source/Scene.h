#pragma once
#include <glm.hpp>
#include <cstdint>
#include <vector>

namespace Lite
{
	// CPU 上的一份顶点；两个 API 原封不动上传同样的数据。
	struct Vertex
	{
		glm::vec3 m_position, m_normal;
		glm::vec2 m_uv;
	};
	struct MeshData
	{
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};
	// 每帧变化的数据放 UBO/CBV，物体变化的数据放 push/root constants。
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
	static_assert(sizeof(Vertex) == 32 && sizeof(FrameConstants) == 96 && sizeof(DrawConstants) == 80,
				  "CPU / HLSL layout must match");
	MeshData MakeDemoMesh();
	FrameData MakeDemoFrame(const glm::vec3 &i_eye, const glm::vec3 &i_forward, float i_aspect, float i_time);
} // namespace Lite
