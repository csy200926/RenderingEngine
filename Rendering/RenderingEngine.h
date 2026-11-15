#pragma once
#include "ISingleton.h"
#include "Backend/IRenderBackend.h"
namespace MyEngine
{
	class Scene;
}
namespace Rendering
{
	class LightBase;
	class RenderingEngine : public Singleton<RenderingEngine>
	{
	  public:
		~RenderingEngine();
		void Initilize(RenderMode i_mode, void *i_pWindow, int i_width, int i_height, int i_samples,
					   bool i_warp = false);
		void SetEnvironment(TextureCubePtr i_sky, TextureCubePtr i_ambient = {});
		void Render(MyEngine::Scene &i_scene, const Camera &i_camera, const std::filesystem::path &i_capture = {},
					bool i_vsync = true);
		void Submit(const DrawItem &i_item) { m_frame.m_items.push_back(i_item); }
		void AddLight(const LightBase &i_light);
		void Resize(int i_width, int i_height);
		void SetDrawSky(bool i_enabled) { m_frame.m_drawSky = i_enabled; }
		void SetShadows(bool i_enabled) { m_shadows = i_enabled; }
		void ShutDown();
		void SetProfiling(bool i_enabled) { m_backend->SetProfiling(i_enabled); }
		double GetGPUTimeMilliseconds() const { return m_backend->GetGPUTimeMilliseconds(); }

	  private:
		std::unique_ptr<IRenderBackend> m_backend;
		RenderFrame m_frame;
		int m_lightCount = 0;
		bool m_shadows = true;
	};
} // namespace Rendering
