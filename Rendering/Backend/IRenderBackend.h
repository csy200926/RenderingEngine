#pragma once
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"
#include "Rendering/TextureCube.h"
#include "Rendering/Camera.h"
#include <filesystem>
namespace Rendering
{
	enum class RenderMode
	{
		OpenGL,
		DX11,
		DX12,
		Vulkan
	};
	struct DrawItem
	{
		MeshPtr m_mesh;
		MaterialPtr m_material;
		glm::mat4 m_transform = glm::mat4(1);
		unsigned m_submesh = 0;
		bool m_sprite = false;
	};
	struct PointLightData
	{
		glm::vec4 m_position = glm::vec4(0), m_color = glm::vec4(0), m_attenuation = glm::vec4(.2f, .03f, .01f, 0);
	};
	struct RenderFrame
	{
		Camera m_camera;
		glm::mat4 m_spriteProjection = glm::mat4(1);
		glm::vec3 m_sunDirection = glm::vec3(-1, -1, -1), m_sunColor = glm::vec3(.6f);
		std::array<PointLightData, 4> m_pointLights;
		std::vector<DrawItem> m_items;
		TextureCubePtr m_sky, m_irradiance;
		MeshPtr m_skyMesh;
		int m_width = 1, m_height = 1;
		bool m_drawSky = true;
		// 相机附近共用一张太阳阴影图；矩阵保持引擎统一的 OpenGL 深度范围。
		glm::mat4 m_lightViewProjection = glm::mat4(1);
		glm::vec4 m_shadowParams = glm::vec4(1.f / 2048, .00015f, 1, 1);
	};
	struct alignas(16) DrawConstants
	{
		glm::mat4 m_model, m_view, m_projection, m_normal;
		glm::vec4 m_viewPosition, m_color, m_material, m_surface, m_sunDirection, m_sunColor;
		glm::vec4 m_pointPosition[4], m_pointColor[4], m_pointAttenuation[4];
		glm::mat4 m_lightViewProjection;
		glm::vec4 m_shadowParams;
	};
	static_assert(sizeof(DrawConstants) == 624, "HLSL constant buffer layout changed");
	DrawConstants MakeConstants(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode);
	void PrepareShadows(RenderFrame &i_frame, bool i_enabled);
	bool CastsShadow(const RenderFrame &i_frame, const DrawItem &i_item);
	class IRenderBackend
	{
	  public:
		virtual ~IRenderBackend() = default;
		virtual void Initilize(void *i_pWindow, int i_width, int i_height, int i_samples, bool i_warp) = 0;
		virtual void Resize(int i_width, int i_height) = 0;
		virtual void Render(const RenderFrame &i_frame) = 0;
		virtual void CaptureFrame(const std::filesystem::path &i_path) = 0;
		virtual void Present(bool i_vsync) = 0;
		virtual void ShutDown() = 0;
		virtual void SetProfiling(bool i_enabled) = 0;
		virtual double GetGPUTimeMilliseconds() const = 0;
	};
	std::unique_ptr<IRenderBackend> CreateOpenGLBackend();
	std::unique_ptr<IRenderBackend> CreateDX11Backend();
	std::unique_ptr<IRenderBackend> CreateDX12Backend();
	std::unique_ptr<IRenderBackend> CreateVulkanBackend();
} // namespace Rendering
