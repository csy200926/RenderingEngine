#include "Director.h"
#include "SceneFile.h"
#include "InputManager.h"
#include "Collision.h"
#include "MessageManager.h"
#include "Rendering/MeshManager.h"
#include "Rendering/MaterialManager.h"
#include "Rendering/TextureManager.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include <lua.hpp>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <algorithm>
namespace MyEngine
{
	EngineSettings LoadSettings()
	{
		EngineSettings result;
		std::unique_ptr<lua_State, decltype(&lua_close)> state(luaL_newstate(), lua_close);
		auto *pState = state.get();
		if (!pState)
			throw std::bad_alloc();
		auto text = Platform::ReadText(Platform::GetAssetPath("Config/Config.lua"));
		if (luaL_loadbuffer(pState, text.data(), text.size(), "Config.lua") || lua_pcall(pState, 0, 0, 0))
			throw std::runtime_error(lua_tostring(pState, -1));
		lua_getglobal(pState, "Settings");
		if (!lua_istable(pState, -1))
			throw std::runtime_error("Config.lua requires a Settings table");
		auto string = [&](const char *key, std::string fallback)
		{
			lua_getfield(pState, -1, key);
			if (lua_isstring(pState, -1))
				fallback = lua_tostring(pState, -1);
			lua_pop(pState, 1);
			return fallback;
		};
		auto number = [&](const char *key, int fallback)
		{
			lua_getfield(pState, -1, key);
			if (lua_isnumber(pState, -1))
				fallback = static_cast<int>(lua_tointeger(pState, -1));
			lua_pop(pState, 1);
			return fallback;
		};
		result.m_windowName = string("WindowName", result.m_windowName);
		result.m_renderer = string("Renderer", result.m_renderer);
		result.m_scene = string("Scene", result.m_scene);
		result.m_model = string("Model", result.m_model);
		result.m_instances = number("Instances", result.m_instances);
		result.m_width = number("Width", result.m_width);
		result.m_height = number("Height", result.m_height);
		result.m_samples = number("Samples", result.m_samples);
		lua_getfield(pState, -1, "VSync");
		if (lua_isboolean(pState, -1))
			result.m_vsync = lua_toboolean(pState, -1) != 0;
		return result;
	}
	Director::Director(const EngineSettings &i_settings) : m_settings(i_settings)
	{
		if (s_instance)
			throw std::logic_error("Only one Director may run");
		s_instance = this;
	}
	Director::~Director()
	{
		m_running = false;
		currentScene.reset();
		m_pendingScene.reset();
		Rendering::RenderingEngine::DestroyInstance();
		Rendering::MaterialManager::DestroyInstance();
		Rendering::MeshManager::DestroyInstance();
		Rendering::TextureManager::DestroyInstance();
		MessageManager::DestroyInstance();
		Collision::DestroyInstance();
		// Window destructor may still dispatch focus messages; InputManager is destroyed in main after Window.
		s_instance = nullptr;
	}
	void Director::Initilize()
	{
		std::wstring title = Platform::ToWide(m_settings.m_windowName);
		title += L" | " + Platform::ToWide(m_settings.m_renderer);
		title += m_settings.m_scene == "game" ? L" | W/S aim, J fire, K super, Esc exit"
											  : L" | WASD move, RMB look, Esc exit";
		m_window.Initilize(title, m_settings.m_width, m_settings.m_height);
		auto mode = Rendering::RenderMode::OpenGL;
		if (m_settings.m_renderer == "dx11")
			mode = Rendering::RenderMode::DX11;
		else if (m_settings.m_renderer == "dx12")
			mode = Rendering::RenderMode::DX12;
		else if (m_settings.m_renderer == "vulkan")
			mode = Rendering::RenderMode::Vulkan;
		Rendering::RenderingEngine::GetInstance()->Initilize(mode, m_window.GetHandle(), m_settings.m_width,
															 m_settings.m_height, m_settings.m_samples,
															 m_settings.m_warp);
		auto sky = std::make_shared<Rendering::TextureCube>();
		sky->Init("Images/Rendering/skyBox/stormydays_ft.tga", "Images/Rendering/skyBox/stormydays_bk.tga",
				  "Images/Rendering/skyBox/stormydays_up.tga", "Images/Rendering/skyBox/stormydays_dn.tga",
				  "Images/Rendering/skyBox/stormydays_lf.tga", "Images/Rendering/skyBox/stormydays_rt.tga");
		Rendering::RenderingEngine::GetInstance()->SetEnvironment(sky);
		Rendering::RenderingEngine::GetInstance()->SetProfiling(m_settings.m_profile);
		Rendering::RenderingEngine::GetInstance()->SetShadows(m_settings.m_shadows);
		Rendering::RenderingEngine::GetInstance()->SetDrawSky(m_settings.m_scene != "game");
		m_camera.SetLookAt(glm::vec3(6, 4, 9), glm::vec3(0, .5f, 0), glm::vec3(0, 1, 0));
		m_camera.SetPerspective(55, static_cast<float>(m_settings.m_width) / m_settings.m_height, .1f, 200.f);
	}
	void Director::pushScene(std::unique_ptr<Scene> i_scene)
	{
		if (m_running)
		{
			m_pendingScene = std::move(i_scene);
			m_hasPendingScene = true;
		}
		else
			currentScene = std::move(i_scene);
	}
	void Director::SetCameraProjection(float i_near, float i_far)
	{
		m_cameraNear = i_near;
		m_cameraFar = i_far;
		m_camera.SetPerspective(m_cameraFov, static_cast<float>(m_window.GetWidth()) / m_window.GetHeight(), i_near,
								i_far);
	}
	void Director::ApplySceneFileSettings(const SceneFileSettings &i_settings)
	{
		// 先完整加载六张天空图片，再应用相机；资源错误不会留下半份新设置。
		auto sky = std::make_shared<Rendering::TextureCube>();
		const auto &faces = i_settings.skyFaces;
		sky->Init(faces[0].c_str(), faces[1].c_str(), faces[2].c_str(), faces[3].c_str(), faces[4].c_str(),
				  faces[5].c_str());
		auto renderer = Rendering::RenderingEngine::GetInstance();
		Rendering::TextureCubePtr ambient;
		if (!i_settings.ambientFaces[0].empty())
		{
			ambient = std::make_shared<Rendering::TextureCube>();
			const auto &a = i_settings.ambientFaces;
			ambient->Init(a[0].c_str(), a[1].c_str(), a[2].c_str(), a[3].c_str(), a[4].c_str(), a[5].c_str());
		}
		renderer->SetEnvironment(sky, ambient);
		renderer->SetDrawSky(i_settings.drawSky);
		m_cameraFov = i_settings.cameraFov;
		m_camera.SetLookAt(i_settings.cameraEye, i_settings.cameraEye + i_settings.cameraDirection, glm::vec3(0, 1, 0));
		SetCameraProjection(i_settings.cameraNear, i_settings.cameraFar);
	}
	void Director::addActor(const ActorPtr &i_actor)
	{
		if (!currentScene)
			throw std::logic_error("No scene");
		currentScene->addActor(i_actor);
	}
	bool Director::isKeyDown(unsigned i_key) const
	{
		return InputManager::GetInstance()->isKeyDown(i_key);
	}
	bool Director::isKeyPressed(unsigned i_key) const
	{
		return InputManager::GetInstance()->isKeyPressed(i_key);
	}
	void Director::UpdateCamera(double i_delta)
	{
		if (m_settings.m_scene == "game" && m_settings.m_sceneFile.empty())
			return;
		auto *pInput = InputManager::GetInstance();
		float speed = static_cast<float>(i_delta) * (pInput->isKeyDown(VK_SHIFT) ? 10.f : 4.f);
		auto forward = m_camera.GetForwardDir(), right = m_camera.GetLeftDir();
		if (pInput->isKeyDown('W'))
			m_camera.cameraPos += forward * speed;
		if (pInput->isKeyDown('S'))
			m_camera.cameraPos -= forward * speed;
		if (pInput->isKeyDown('A'))
			m_camera.cameraPos -= right * speed;
		if (pInput->isKeyDown('D'))
			m_camera.cameraPos += right * speed;
		if (pInput->isKeyDown('Q'))
			m_camera.cameraPos.y -= speed;
		if (pInput->isKeyDown('E'))
			m_camera.cameraPos.y += speed;
		if (pInput->isKeyDown(VK_RBUTTON))
		{
			auto delta = pInput->GetMouseDelta();
			float pitch = std::asin(std::clamp(m_camera.viewVector.y, -1.f, 1.f));
			float yaw = std::atan2(m_camera.viewVector.z, m_camera.viewVector.x);
			yaw += delta.x * .004f;
			pitch = std::clamp(pitch - delta.y * .004f, -1.5f, 1.5f);
			m_camera.viewVector =
				glm::vec3(std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw));
		}
		m_camera.Update();
	}
	void Director::run()
	{
		using Clock = std::chrono::steady_clock;
		auto last = Clock::now();
		int frame = 0;
		m_running = true;
		auto *pInput = InputManager::GetInstance();
		auto *pRenderer = Rendering::RenderingEngine::GetInstance();
		double renderMilliseconds = 0;
		double gpuMilliseconds = 0;
		int gpuFrames = 0;
		int timedFrames = 0;
		while (m_running)
		{
			auto start = Clock::now();
			double delta = std::chrono::duration<double>(start - last).count();
			last = start;
			pInput->update();
			if (!m_window.PumpEvents() || pInput->isKeyPressed(VK_ESCAPE))
				break;
			if (m_window.IsMinimized() || m_window.GetWidth() < 1 || m_window.GetHeight() < 1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				last = Clock::now();
				continue;
			}
			if (m_window.TakeResize())
			{
				pRenderer->Resize(m_window.GetWidth(), m_window.GetHeight());
				m_camera.SetPerspective(m_cameraFov, static_cast<float>(m_window.GetWidth()) / m_window.GetHeight(),
										m_cameraNear, m_cameraFar);
				Platform::Log("Window resized: " + std::to_string(m_window.GetWidth()) + "x" +
							  std::to_string(m_window.GetHeight()));
			}
			if (m_hasPendingScene)
			{
				currentScene = std::move(m_pendingScene);
				m_hasPendingScene = false;
			}
			if (!currentScene)
				break;
			delta = m_settings.m_frameLimit > 0 ? 1.0 / 60.0 : std::clamp(delta, 0.0, .05);
			UpdateCamera(delta);
			currentScene->InternalUpdate(delta);
			++frame;
			bool capture = !m_settings.m_capture.empty() &&
						   (m_settings.m_frameLimit == 0 ? frame == 1 : frame == m_settings.m_frameLimit);
			auto renderStart = Clock::now();
			pRenderer->Render(*currentScene, m_camera, capture ? m_settings.m_capture : std::filesystem::path(),
							  m_settings.m_vsync);
			if (frame > 30 && !capture)
			{
				renderMilliseconds += std::chrono::duration<double, std::milli>(Clock::now() - renderStart).count();
				++timedFrames;
				double gpu = pRenderer->GetGPUTimeMilliseconds();
				if (gpu >= 0)
				{
					gpuMilliseconds += gpu;
					++gpuFrames;
				}
			}
			if (m_settings.m_frameLimit > 0 && frame >= m_settings.m_frameLimit)
				break;
			if (!m_settings.m_vsync && m_settings.m_frameLimit == 0)
				std::this_thread::sleep_until(start + std::chrono::milliseconds(8));
		}
		m_running = false;
		if (gpuFrames)
			Platform::Log("GPU timing: mean-ms=" + std::to_string(gpuMilliseconds / gpuFrames) + ", samples=" +
						  std::to_string(gpuFrames) + ", hardware queries, synchronous profiling enabled");
		if (timedFrames)
			Platform::Log("Frame timing: warmed-render-present-ms=" + std::to_string(renderMilliseconds / timedFrames) +
						  ", rate=" + std::to_string(timedFrames * 1000.0 / renderMilliseconds) + ", samples=" +
						  std::to_string(timedFrames) + ", vsync=" + (m_settings.m_vsync ? "on" : "off") +
						  " (CPU/driver wall time, not GPU query time)");
		Platform::Log("Finished: frames=" + std::to_string(frame) +
					  ", actors=" + std::to_string(currentScene ? currentScene->GetActors().len : 0) +
					  ", collisions=" + std::to_string(Collision::GetInstance()->GetCollisionCount()) +
					  ", message-deliveries=" + std::to_string(MessageManager::GetInstance()->GetDeliveryCount()));
	}
} // namespace MyEngine
