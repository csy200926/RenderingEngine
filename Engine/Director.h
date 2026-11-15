#pragma once
#include "Scene.h"
#include "Rendering/Camera.h"
#include "Rendering/RenderingEngine.h"
#include "Platform/Window.h"
#include <filesystem>
#include <memory>
namespace MyEngine
{
	struct SceneFileSettings;
	struct EngineSettings
	{
		std::string m_windowName = "MyEngine", m_renderer = "opengl", m_scene = "rendering";
		std::string m_model;
		int m_instances = 1;
		int m_width = 1280, m_height = 800, m_samples = 4, m_frameLimit = 0;
		bool m_vsync = true, m_warp = false, m_autoplay = false;
		bool m_profile = false;
		bool m_shadows = true;
		std::filesystem::path m_capture;
		std::filesystem::path m_sceneFile;
	};
	EngineSettings LoadSettings();
	class Director
	{
	  public:
		explicit Director(const EngineSettings &i_settings);
		~Director();
		static Director *sharedDirector() { return s_instance; }
		void Initilize();
		void run();
		void pushScene(std::unique_ptr<Scene> i_scene);
		void addActor(const ActorPtr &i_actor);
		bool isKeyDown(unsigned i_key) const;
		bool isKeyPressed(unsigned i_key) const;
		Rendering::Camera &GetCamera() { return m_camera; }
		void SetCameraProjection(float i_near, float i_far);
		void ApplySceneFileSettings(const SceneFileSettings &i_settings);

	  private:
		inline static Director *s_instance = nullptr;
		EngineSettings m_settings;
		Platform::Window m_window;
		std::unique_ptr<Scene> currentScene, m_pendingScene;
		Rendering::Camera m_camera;
		bool m_running = false, m_hasPendingScene = false;
		float m_cameraFov = 55, m_cameraNear = .1f, m_cameraFar = 200.f;
		void UpdateCamera(double i_delta);
	};
} // namespace MyEngine
