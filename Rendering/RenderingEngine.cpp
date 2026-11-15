#include "RenderingEngine.h"
#include "Engine/Scene.h"
#include "Lighting.h"
#include "MeshRenderer.h"
#include "SpriteRenderer.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include <gtc/matrix_transform.hpp>
namespace Rendering
{
	RenderingEngine::~RenderingEngine()
	{
		ShutDown();
	}
	void RenderingEngine::Initilize(RenderMode i_mode, void *i_pWindow, int i_width, int i_height, int i_samples,
									bool i_warp)
	{
		// 场景和材质完全相同，只有这里选择实际调用哪个图形 API。
		switch (i_mode)
		{
		case RenderMode::OpenGL:
			m_backend = CreateOpenGLBackend();
			break;
		case RenderMode::DX11:
			m_backend = CreateDX11Backend();
			break;
		case RenderMode::DX12:
			m_backend = CreateDX12Backend();
			break;
		case RenderMode::Vulkan:
			m_backend = CreateVulkanBackend();
			break;
		default:
			throw std::invalid_argument("Unknown render backend");
		}
		m_backend->Initilize(i_pWindow, i_width, i_height, i_samples, i_warp);
		m_frame.m_width = i_width;
		m_frame.m_height = i_height;
		m_frame.m_skyMesh = MeshManager::GetInstance()->GetByName("Default");
	}
	void RenderingEngine::SetEnvironment(TextureCubePtr i_sky, TextureCubePtr i_ambient)
	{
		m_frame.m_sky = std::move(i_sky);
		// 天空背景和物体环境光分开：源场景指定三色环境时，不能悄悄换成蓝色天空卷积。
		m_frame.m_irradiance = i_ambient ? std::move(i_ambient) : m_frame.m_sky->ConvolveIrradiance();
	}
	void RenderingEngine::Resize(int i_width, int i_height)
	{
		if (i_width < 1 || i_height < 1)
			return;
		m_frame.m_width = i_width;
		m_frame.m_height = i_height;
		m_backend->Resize(i_width, i_height);
	}
	void RenderingEngine::Render(MyEngine::Scene &i_scene, const Camera &i_camera,
								 const std::filesystem::path &i_capture, bool i_vsync)
	{
		m_frame.m_camera = i_camera;
		m_frame.m_items.clear();
		m_lightCount = 0;
		m_frame.m_sunColor = glm::vec3(0);
		for (auto &light : m_frame.m_pointLights)
			light.m_color = glm::vec4(0);
		// Original game coordinates are 800x600 centered, independent of viewport size.
		m_frame.m_spriteProjection = glm::ortho(-400.f, 400.f, -300.f, 300.f, -1000.f, 1000.f);
		i_scene.Draw(*this);
		PrepareShadows(m_frame, m_shadows);
		std::stable_partition(m_frame.m_items.begin(), m_frame.m_items.end(),
							  [](const auto &item) { return !item.m_sprite; });
		auto transparent =
			std::stable_partition(m_frame.m_items.begin(), m_frame.m_items.end(), [](const auto &item)
								  { return !item.m_sprite && !(item.m_material && item.m_material->m_transparent); });
		auto sprites = std::find_if(transparent, m_frame.m_items.end(), [](const auto &item) { return item.m_sprite; });
		std::stable_sort(
			transparent, sprites,
			[&](const DrawItem &a, const DrawItem &b)
			{
				auto center = [](const DrawItem &item)
				{
					auto &part = item.m_mesh->m_subMeshes[item.m_submesh];
					return glm::vec3(item.m_transform * glm::vec4((part.m_minimum + part.m_maximum) * .5f, 1));
				};
				return glm::length(center(a) - i_camera.cameraPos) > glm::length(center(b) - i_camera.cameraPos);
			});
		m_backend->Render(m_frame);
		if (!i_capture.empty())
			m_backend->CaptureFrame(i_capture);
		m_backend->Present(i_vsync);
		m_frame.m_items.clear();
	}
	void RenderingEngine::AddLight(const LightBase &i_light)
	{
		if (i_light.GetType() == DIRECTIONAL)
		{
			m_frame.m_sunDirection = i_light.m_direction;
			m_frame.m_sunColor = i_light.m_diffuse * i_light.m_intensity;
		}
		else if (i_light.GetType() == POINT && m_lightCount < 4)
		{
			auto &light = m_frame.m_pointLights[m_lightCount++];
			light.m_position = glm::vec4(i_light.GetPosition(), 1);
			light.m_color = glm::vec4(i_light.m_diffuse * i_light.m_intensity, 1);
			light.m_attenuation = glm::vec4(i_light.m_constant, i_light.m_linear, i_light.m_quadratic, 0);
		}
	}
	void RenderingEngine::ShutDown()
	{
		if (m_backend)
		{
			m_backend->ShutDown();
			m_backend.reset();
		}
		m_frame = RenderFrame();
	}
	void MeshRenderer::Render(RenderingEngine &i_engine)
	{
		for (unsigned i = 0; i < m_pMesh->m_subMeshes.size(); ++i)
		{
			MaterialPtr material;
			if (!m_materials.empty())
				material = m_materials[std::min<size_t>(i, m_materials.size() - 1)];
			else
			{
				auto index = m_pMesh->m_subMeshes[i].m_materialIndex;
				material =
					index < m_pMesh->m_materials.size() ? m_pMesh->m_materials[index] : std::make_shared<Material>();
			}
			i_engine.Submit({m_pMesh, material, GetTransform(), i, false});
		}
	}
	void SpriteRenderer::Render(RenderingEngine &i_engine)
	{
		auto material = std::make_shared<Material>();
		material->SetTexture(m_texture);
		material->m_color = m_color;
		auto transform = GetTransform() * glm::scale(glm::mat4(1), glm::vec3(m_size, 1));
		i_engine.Submit({MeshManager::GetInstance()->GetByName("Quad"), material, transform, 0, true});
	}
	glm::vec3 LightBase::GetPosition() const
	{
		return glm::vec3(GetTransform() * glm::vec4(0, 0, 0, 1));
	}
	void LightBase::Render(RenderingEngine &i_engine)
	{
		i_engine.AddLight(*this);
	}
} // namespace Rendering
