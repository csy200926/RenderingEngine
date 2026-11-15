#pragma once
#include "Scene.h"
#include <array>
#include <filesystem>
#include <memory>
namespace MyEngine
{
	// 相机和天空是场景文件数据，不依赖 Editor 或 GPU 后端。
	struct SceneFileSettings
	{
		glm::vec3 cameraEye = glm::vec3(6, 4, 9);
		glm::vec3 cameraDirection = glm::normalize(glm::vec3(-6, -3.5f, -9));
		float cameraFov = 55, cameraNear = .1f, cameraFar = 200;
		bool drawSky = true;
		// 与 TextureCube::Init 参数一致：前、后、上、下、左、右。
		std::array<std::string, 6> skyFaces = {
			"Images/Rendering/skyBox/stormydays_ft.tga", "Images/Rendering/skyBox/stormydays_bk.tga",
			"Images/Rendering/skyBox/stormydays_up.tga", "Images/Rendering/skyBox/stormydays_dn.tga",
			"Images/Rendering/skyBox/stormydays_lf.tga", "Images/Rendering/skyBox/stormydays_rt.tga"};
		// 可选的源环境光采样，顺序同天空六面；为空时沿用天空卷积。
		std::array<std::string, 6> ambientFaces = {};
	};
	struct SceneFileData
	{
		std::unique_ptr<Scene> scene;
		SceneFileSettings settings;
	};
	// 失败抛出可读异常；加载成功才返回新场景，调用方自行替换旧场景。
	SceneFileData LoadSceneFile(const std::filesystem::path &i_path);
	void SaveSceneFile(const Scene &i_scene, const std::filesystem::path &i_path,
					   const SceneFileSettings &i_settings = {});
} // namespace MyEngine
