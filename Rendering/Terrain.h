#pragma once
#include "Mesh.h"
namespace Rendering
{
	struct TerrainData
	{
		unsigned resX = 0, resZ = 0;
		float sizeX = 0, sizeZ = 0;
		std::string albedo;
		std::vector<float> heights;
		bool dirty = false;
	};
	void LoadTerrain(Mesh &o_mesh, const std::filesystem::path &i_path);
	void SaveTerrain(Mesh &io_mesh, const std::string &i_assetPath);
	void RebuildTerrain(Mesh &io_mesh);
	bool BrushTerrain(Mesh &io_mesh, const glm::vec3 &i_localPoint, float i_radius, float i_amount, bool i_smooth);
	bool RaycastTerrain(const Mesh &i_mesh, const glm::vec3 &i_eye, const glm::vec3 &i_ray, glm::vec3 &o_hit);
} // namespace Rendering
