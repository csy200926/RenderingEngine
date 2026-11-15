#include "IRenderBackend.h"
#include <gtc/matrix_transform.hpp>
#include <gtc/matrix_inverse.hpp>
namespace Rendering
{
	void PrepareShadows(RenderFrame &i_frame, bool i_enabled)
	{
		// 第一步：只处理太阳，没太阳或关闭时不画阴影。零方向不能拿去 normalize。
		bool sun = glm::dot(i_frame.m_sunDirection, i_frame.m_sunDirection) > .000001f &&
				   glm::dot(i_frame.m_sunColor, i_frame.m_sunColor) > .000001f;
		i_frame.m_shadowParams.w = i_enabled && sun ? 1.f : 0.f;
		if (!i_frame.m_shadowParams.w)
			return;
		// 第二步：覆盖相机周围及前方约一百米，远山不占这张图的精度。
		auto direction = glm::normalize(i_frame.m_sunDirection);
		auto center = i_frame.m_camera.cameraPos + glm::normalize(i_frame.m_camera.viewVector) * 30.f;
		auto up = std::abs(direction.y) > .95f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
		auto view = glm::lookAt(center - direction * 200.f, center, up);
		i_frame.m_lightViewProjection = glm::ortho(-70.f, 70.f, -70.f, 70.f, .1f, 400.f) * view;
		// 第三步：对齐到阴影像素，减少慢速移动相机时阴影边缘游动。
		auto origin = i_frame.m_lightViewProjection * glm::vec4(0, 0, 0, 1);
		glm::vec2 texels = glm::vec2(origin) / (2.f * i_frame.m_shadowParams.x);
		auto offset = (glm::round(texels) - texels) * (2.f * i_frame.m_shadowParams.x);
		i_frame.m_lightViewProjection[3][0] += offset.x;
		i_frame.m_lightViewProjection[3][1] += offset.y;
	}
	bool CastsShadow(const RenderFrame &i_frame, const DrawItem &i_item)
	{
		// 普通透明玻璃、水和二维精灵不写不透明阴影；树叶的 cutout 材质仍参与。
		if (!i_frame.m_shadowParams.w || i_item.m_sprite || !i_item.m_mesh ||
			(i_item.m_material && i_item.m_material->m_transparent))
			return false;
		const auto &part = i_item.m_mesh->m_subMeshes[i_item.m_submesh];
		auto matrix = i_frame.m_lightViewProjection * i_item.m_transform;
		bool outside[6] = {true, true, true, true, true, true};
		for (int i = 0; i < 8; ++i)
		{
			glm::vec3 corner((i & 1) ? part.m_maximum.x : part.m_minimum.x,
							 (i & 2) ? part.m_maximum.y : part.m_minimum.y,
							 (i & 4) ? part.m_maximum.z : part.m_minimum.z);
			auto p = matrix * glm::vec4(corner, 1);
			outside[0] &= p.x < -p.w;
			outside[1] &= p.x > p.w;
			outside[2] &= p.y < -p.w;
			outside[3] &= p.y > p.w;
			outside[4] &= p.z < -p.w;
			outside[5] &= p.z > p.w;
		}
		for (bool value : outside)
			if (value)
				return false;
		return true;
	}
	DrawConstants MakeConstants(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode)
	{
		DrawConstants result = {};
		result.m_model = i_item.m_transform;
		result.m_view = i_mode == 1 ? glm::mat4(1) : i_frame.m_camera.WorldToView_Matrix;
		result.m_projection = i_mode == 1 ? i_frame.m_spriteProjection : i_frame.m_camera.Projective_Matrix;
		result.m_normal = glm::inverseTranspose(result.m_model);
		result.m_lightViewProjection = i_frame.m_lightViewProjection;
		result.m_shadowParams = i_frame.m_shadowParams;
		result.m_viewPosition = glm::vec4(i_frame.m_camera.cameraPos, 1);
		result.m_color = i_item.m_material ? i_item.m_material->m_color : glm::vec4(1);
		auto *pMat = i_item.m_material.get();
		result.m_surface = glm::vec4(pMat ? pMat->m_alphaCutoff : 0, pMat && pMat->m_opacityTexture ? 1.f : 0.f,
									 pMat && pMat->m_normalTexture ? 1.f : 0.f, 0);
		result.m_material =
			glm::vec4(pMat ? pMat->m_metallic : 0, pMat ? glm::clamp(pMat->m_roughness, .045f, 1.f) : .5f,
					  pMat ? pMat->m_ao : 1, static_cast<float>(i_mode));
		result.m_sunDirection = glm::vec4(glm::normalize(i_frame.m_sunDirection), 0);
		result.m_sunColor = glm::vec4(i_frame.m_sunColor, 0);
		for (int i = 0; i < 4; ++i)
		{
			result.m_pointPosition[i] = i_frame.m_pointLights[i].m_position;
			result.m_pointColor[i] = i_frame.m_pointLights[i].m_color;
			result.m_pointAttenuation[i] = i_frame.m_pointLights[i].m_attenuation;
		}
		return result;
	}
} // namespace Rendering
