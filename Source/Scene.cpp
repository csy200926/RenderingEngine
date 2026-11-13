#include "Scene.h"
#include <gtc/matrix_transform.hpp>

namespace Rendering
{
	static void AddFace(MeshData& io_mesh, glm::vec3 i_a, glm::vec3 i_b,
		glm::vec3 i_c, glm::vec3 i_d, glm::vec3 i_normal)
	{
		uint32_t first = (uint32_t)io_mesh.m_vertices.size();
		io_mesh.m_vertices.push_back({i_a, i_normal, {0, 0}});
		io_mesh.m_vertices.push_back({i_b, i_normal, {1, 0}});
		io_mesh.m_vertices.push_back({i_c, i_normal, {1, 1}});
		io_mesh.m_vertices.push_back({i_d, i_normal, {0, 1}});

		uint32_t indices[] = {first, first + 1, first + 2, first, first + 2, first + 3};
		for(int i = 0; i < 6; i++)
			io_mesh.m_indices.push_back(indices[i]);
	}

	MeshData MakeDemoMesh()
	{
		MeshData mesh;
		// 每个面单独留四个顶点，否则边上的法线会混在一起。
		AddFace(mesh, {-.5f, -.5f, .5f}, {.5f, -.5f, .5f}, {.5f, .5f, .5f}, {-.5f, .5f, .5f}, {0, 0, 1});
		AddFace(mesh, {.5f, -.5f, -.5f}, {-.5f, -.5f, -.5f}, {-.5f, .5f, -.5f}, {.5f, .5f, -.5f}, {0, 0, -1});
		AddFace(mesh, {.5f, -.5f, .5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {.5f, .5f, .5f}, {1, 0, 0});
		AddFace(mesh, {-.5f, -.5f, -.5f}, {-.5f, -.5f, .5f}, {-.5f, .5f, .5f}, {-.5f, .5f, -.5f}, {-1, 0, 0});
		AddFace(mesh, {-.5f, .5f, .5f}, {.5f, .5f, .5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f}, {0, 1, 0});
		AddFace(mesh, {-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, -.5f, .5f}, {-.5f, -.5f, .5f}, {0, -1, 0});


		// 地面接在方块后面，画的时候换 firstIndex，不再建一套缓冲。
		AddFace(mesh, {-6, 0, 6}, {6, 0, 6}, {6, 0, -6}, {-6, 0, -6}, {0, 1, 0});
		return mesh;
	}

	FrameData MakeDemoFrame(const Camera& i_camera, float i_time)
	{
		FrameData frame;
		frame.m_constants.m_viewProjection = i_camera.Projective_Matrix * i_camera.WorldToView_Matrix;
		frame.m_constants.m_cameraPosition = glm::vec4(i_camera.cameraPos, 1);

		DrawItem cube;
		cube.m_indexCount = 36;
		glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0, 1, 0));
		model = glm::rotate(model, 0.45f + i_time * 0.4f, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(2));
		cube.m_constants.m_model = model;
		cube.m_constants.m_color = glm::vec4(.82f, .30f, .09f, 1);
		frame.m_items.push_back(cube);


		DrawItem ground;
		ground.m_firstIndex = 36;
		ground.m_indexCount = 6;
		ground.m_constants.m_color = glm::vec4(.22f, .35f, .43f, 1);
		frame.m_items.push_back(ground);
		return frame;
	}
}
