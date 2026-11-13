#include "Scene.h"
#include <gtc/matrix_transform.hpp>

namespace Lite
{
	MeshData MakeDemoMesh()
	{
		MeshData mesh;
		// 沿用原内建方块的做法：六个面各四个顶点，面之间不共用法线。
		auto face = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 normal)
		{
			auto first = static_cast<uint32_t>(mesh.m_vertices.size());
			mesh.m_vertices.insert(
				mesh.m_vertices.end(),
				{{a, normal, {0, 0}}, {b, normal, {1, 0}}, {c, normal, {1, 1}}, {d, normal, {0, 1}}});
			mesh.m_indices.insert(mesh.m_indices.end(), {first, first + 1, first + 2, first, first + 2, first + 3});
		};
		face({-.5f, -.5f, .5f}, {.5f, -.5f, .5f}, {.5f, .5f, .5f}, {-.5f, .5f, .5f}, {0, 0, 1});
		face({.5f, -.5f, -.5f}, {-.5f, -.5f, -.5f}, {-.5f, .5f, -.5f}, {.5f, .5f, -.5f}, {0, 0, -1});
		face({.5f, -.5f, .5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {.5f, .5f, .5f}, {1, 0, 0});
		face({-.5f, -.5f, -.5f}, {-.5f, -.5f, .5f}, {-.5f, .5f, .5f}, {-.5f, .5f, -.5f}, {-1, 0, 0});
		face({-.5f, .5f, .5f}, {.5f, .5f, .5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f}, {0, 1, 0});
		face({-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, -.5f, .5f}, {-.5f, -.5f, .5f}, {0, -1, 0});
		// 地面也放进同一个顶点/索引缓冲，只用 firstIndex 区分两次绘制。
		face({-6, 0, 6}, {6, 0, 6}, {6, 0, -6}, {-6, 0, -6}, {0, 1, 0});
		return mesh;
	}
	FrameData MakeDemoFrame(const glm::vec3 &i_eye, const glm::vec3 &i_forward, float i_aspect, float i_time)
	{
		FrameData frame;
		// 列向量从右往左乘：局部位置 -> Model -> View -> Projection -> 裁剪坐标。
		frame.m_constants.m_viewProjection = glm::perspective(glm::radians(55.f), i_aspect, .1f, 100.f) *
											 glm::lookAt(i_eye, i_eye + i_forward, glm::vec3(0, 1, 0));
		frame.m_constants.m_cameraPosition = glm::vec4(i_eye, 1);
		DrawItem cube;
		cube.m_indexCount = 36;
		cube.m_constants.m_model = glm::translate(glm::mat4(1), glm::vec3(0, 1, 0)) *
								   glm::rotate(glm::mat4(1), .45f + i_time * .4f, glm::vec3(0, 1, 0)) *
								   glm::scale(glm::mat4(1), glm::vec3(2));
		cube.m_constants.m_color = glm::vec4(.82f, .30f, .09f, 1);
		frame.m_items.push_back(cube);
		DrawItem ground;
		ground.m_firstIndex = 36;
		ground.m_indexCount = 6;
		ground.m_constants.m_color = glm::vec4(.22f, .35f, .43f, 1);
		frame.m_items.push_back(ground);
		return frame;
	}
} // namespace Lite
