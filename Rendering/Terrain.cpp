#include "Terrain.h"
#include "TextureManager.h"
#include "Platform/Paths.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>
namespace Rendering
{
	static void CheckTerrain(const TerrainData &t)
	{
		if (t.resX < 2 || t.resZ < 2 || t.resX > 2049 || t.resZ > 2049 || !std::isfinite(t.sizeX) ||
			!std::isfinite(t.sizeZ) || t.sizeX <= .001f || t.sizeZ <= .001f || t.sizeX > 1000000 || t.sizeZ > 1000000 ||
			t.heights.size() != size_t(t.resX) * t.resZ)
			throw std::runtime_error("Terrain: invalid dimensions");
		for (float h : t.heights)
			if (!std::isfinite(h) || std::abs(h) > 1000000)
				throw std::runtime_error("Terrain: invalid height");
	}
	static std::filesystem::path TerrainAsset(const std::string &name)
	{
		if (name.empty() || name.size() > 4096 || name.find('\0') != std::string::npos ||
			!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), int(name.size()), nullptr, 0))
			throw std::runtime_error("Terrain: invalid UTF8 asset path");
		auto path = std::filesystem::u8path(name);
		if (path.is_absolute() || path.has_root_name() || name.find(':') != std::string::npos ||
			name.find('\\') != std::string::npos)
			throw std::runtime_error("Terrain: expected relative Assets path");
		for (const auto &part : path)
			if (part == ".." || part == ".")
				throw std::runtime_error("Terrain: unsafe asset path");
		auto root = std::filesystem::weakly_canonical(Platform::GetAssetPath(""));
		auto full = std::filesystem::weakly_canonical(root / path);
		auto relative = full.lexically_relative(root);
		if (relative.empty() || relative.is_absolute() || *relative.begin() == "..")
			throw std::runtime_error("Terrain: asset escapes Assets");
		return full;
	}
	void LoadTerrain(Mesh &mesh, const std::filesystem::path &path)
	{
		// 第一步：先限制文件长度，再按小端逐字段读取；不把损坏尺寸直接当作分配长度。
		auto length = std::filesystem::file_size(path);
		if (length < 28 || length > 32 * 1024 * 1024)
			throw std::runtime_error("Terrain: invalid file size");
		auto bytes = Platform::ReadBytes(path);
		if (std::memcmp(bytes.data(), "MYTRN01\0", 8))
			throw std::runtime_error("Terrain: invalid signature");
		size_t at = 8;
		auto read = [&]()
		{
			if (at + 4 > bytes.size())
				throw std::runtime_error("Terrain: truncated header");
			uint32_t n = uint32_t(bytes[at]) | uint32_t(bytes[at + 1]) << 8 | uint32_t(bytes[at + 2]) << 16 |
						 uint32_t(bytes[at + 3]) << 24;
			at += 4;
			return n;
		};
		auto real = [&]()
		{
			auto n = read();
			float f;
			std::memcpy(&f, &n, 4);
			return f;
		};
		auto t = std::make_shared<TerrainData>();
		t->resX = read();
		t->resZ = read();
		t->sizeX = real();
		t->sizeZ = real();
		auto names = read();
		if (t->resX < 2 || t->resZ < 2 || t->resX > 2049 || t->resZ > 2049 || names > 4096 ||
			at + names + uint64_t(t->resX) * t->resZ * 4 != bytes.size())
			throw std::runtime_error("Terrain: invalid payload length");
		t->albedo.assign(reinterpret_cast<const char *>(bytes.data() + at), names);
		at += names;
		t->heights.resize(size_t(t->resX) * t->resZ);
		for (auto &h : t->heights)
			h = real();
		CheckTerrain(*t);
		auto material = std::make_shared<Material>();
		material->m_roughness = 1;
		mesh.m_importDependencies.push_back(path);
		if (!t->albedo.empty())
		{
			auto texture = TerrainAsset(t->albedo);
			if (!std::filesystem::is_regular_file(texture))
				throw std::runtime_error("Terrain: missing albedo " + t->albedo);
			mesh.m_importDependencies.push_back(texture);
			material->m_texture = TextureManager::GetInstance()->Load(t->albedo, t->albedo);
		}
		mesh.m_materials.push_back(material);
		mesh.m_terrain = t;
		RebuildTerrain(mesh);
	}
	void RebuildTerrain(Mesh &mesh)
	{
		auto &t = *mesh.m_terrain;
		CheckTerrain(t);
		mesh.m_vertices.resize(t.heights.size());
		SubMesh part;
		part.m_minimum = glm::vec3(1e30f);
		part.m_maximum = glm::vec3(-1e30f);
		// 第二步：高度是局部Y；中心差分法线保持地形边缘也朝上。
		float dx = t.sizeX / (t.resX - 1), dz = t.sizeZ / (t.resZ - 1);
		for (unsigned z = 0; z < t.resZ; ++z)
			for (unsigned x = 0; x < t.resX; ++x)
			{
				auto left = x ? x - 1 : x, right = std::min(x + 1, t.resX - 1);
				auto back = z ? z - 1 : z, front = std::min(z + 1, t.resZ - 1);
				float slopeX = (t.heights[z * t.resX + right] - t.heights[z * t.resX + left]) / ((right - left) * dx);
				float slopeZ = (t.heights[front * t.resX + x] - t.heights[back * t.resX + x]) / ((front - back) * dz);
				auto &v = mesh.m_vertices[z * t.resX + x];
				v = {{x * dx, t.heights[z * t.resX + x], z * dz},
					 glm::normalize(glm::vec3(-slopeX, 1, -slopeZ)),
					 {float(x) / (t.resX - 1), float(z) / (t.resZ - 1)}};
				part.m_minimum = glm::min(part.m_minimum, v._position);
				part.m_maximum = glm::max(part.m_maximum, v._position);
			}
		if (mesh.m_indices.size() != size_t(t.resX - 1) * (t.resZ - 1) * 6)
		{
			mesh.m_indices.clear();
			mesh.m_indices.reserve(size_t(t.resX - 1) * (t.resZ - 1) * 6);
			for (unsigned z = 0; z + 1 < t.resZ; ++z)
				for (unsigned x = 0; x + 1 < t.resX; ++x)
				{
					unsigned a = z * t.resX + x, b = a + 1, c = a + t.resX, d = c + 1;
					for (auto index : {a, c, b, b, c, d})
						mesh.m_indices.push_back(index);
				}
		}
		part.m_indexCount = unsigned(mesh.m_indices.size());
		mesh.m_subMeshes = {part};
		++mesh.m_revision;
	}
	MeshPtr Mesh::CreateTerrain(unsigned res, float size)
	{
		auto mesh = std::make_shared<Mesh>(std::vector<Vertex>{}, std::vector<unsigned>{});
		mesh->m_terrain = std::make_shared<TerrainData>();
		auto &t = *mesh->m_terrain;
		t.resX = t.resZ = res;
		t.sizeX = t.sizeZ = size;
		if (res < 2 || res > 2049)
			throw std::runtime_error("Terrain: invalid resolution");
		t.heights.resize(size_t(res) * res);
		t.dirty = true;
		auto material = std::make_shared<Material>();
		material->m_roughness = 1;
		mesh->m_materials = {material};
		RebuildTerrain(*mesh);
		return mesh;
	}
	MeshPtr Mesh::CloneTerrain() const
	{
		if (!m_terrain)
			throw std::runtime_error("Terrain: mesh is not a terrain");
		auto mesh = std::make_shared<Mesh>(m_vertices, m_indices);
		mesh->m_terrain = std::make_shared<TerrainData>(*m_terrain);
		mesh->m_materials = m_materials;
		mesh->m_subMeshes = m_subMeshes;
		mesh->m_importDependencies = m_importDependencies;
		mesh->m_fileName = m_fileName;
		return mesh;
	}
	void Mesh::SetTerrainFile(const std::string &path)
	{
		m_fileName = path;
		m_importDependencies.clear();
		m_importDependencies.push_back(TerrainAsset(path));
		if (!m_terrain->albedo.empty())
			m_importDependencies.push_back(TerrainAsset(m_terrain->albedo));
	}
	void SaveTerrain(Mesh &mesh, const std::string &name)
	{
		auto &t = *mesh.m_terrain;
		CheckTerrain(t);
		auto path = TerrainAsset(name);
		if (!t.albedo.empty())
			TerrainAsset(t.albedo);
		std::filesystem::create_directories(path.parent_path());
		auto temp = path;
		temp += L".tmp";
		try
		{
			std::ofstream file(temp, std::ios::binary | std::ios::trunc);
			file.write("MYTRN01\0", 8);
			auto write = [&](uint32_t n)
			{
				char b[4] = {char(n), char(n >> 8), char(n >> 16), char(n >> 24)};
				file.write(b, 4);
			};
			auto real = [&](float f)
			{
				uint32_t n;
				std::memcpy(&n, &f, 4);
				write(n);
			};
			write(t.resX);
			write(t.resZ);
			real(t.sizeX);
			real(t.sizeZ);
			write(unsigned(t.albedo.size()));
			file.write(t.albedo.data(), t.albedo.size());
			for (auto h : t.heights)
				real(h);
			file.flush();
			if (!file)
				throw std::runtime_error("Terrain: write failed");
			file.close();
			if (!file || !MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				throw std::runtime_error("Terrain: replace failed");
		}
		catch (...)
		{
			std::error_code error;
			std::filesystem::remove(temp, error);
			throw;
		}
		mesh.SetTerrainFile(name);
	}
	bool BrushTerrain(Mesh &mesh, const glm::vec3 &point, float radius, float amount, bool smooth)
	{
		if (!mesh.m_terrain || !std::isfinite(radius) || radius <= 0 || !std::isfinite(amount))
			return false;
		auto &t = *mesh.m_terrain;
		auto before = t.heights;
		bool changed = false;
		float dx = t.sizeX / (t.resX - 1), dz = t.sizeZ / (t.resZ - 1);
		int x0 = std::max(0, int(std::floor((point.x - radius) / dx))),
			x1 = std::min(int(t.resX) - 1, int(std::ceil((point.x + radius) / dx)));
		int z0 = std::max(0, int(std::floor((point.z - radius) / dz))),
			z1 = std::min(int(t.resZ) - 1, int(std::ceil((point.z + radius) / dz)));
		// 第三步：平滑读取笔刷前的高度，避免扫描方向影响结果。
		for (int z = z0; z <= z1; ++z)
			for (int x = x0; x <= x1; ++x)
			{
				float d = glm::length(glm::vec2(x * dx - point.x, z * dz - point.z));
				if (d >= radius)
					continue;
				float weight = 1 - d / radius;
				weight = weight * weight * (3 - 2 * weight);
				auto index = z * t.resX + x;
				float h = before[index];
				if (smooth)
				{
					float sum = 0;
					int count = 0;
					for (int zz = std::max(0, z - 1); zz <= std::min(int(t.resZ) - 1, z + 1); ++zz)
						for (int xx = std::max(0, x - 1); xx <= std::min(int(t.resX) - 1, x + 1); ++xx)
						{
							sum += before[zz * t.resX + xx];
							++count;
						}
					h += (sum / count - h) * glm::clamp(std::abs(amount) * weight, 0.f, 1.f);
				}
				else
					h += amount * weight;
				h = glm::clamp(h, -1000000.f, 1000000.f);
				if (h != before[index])
				{
					t.heights[index] = h;
					changed = true;
				}
			}
		if (changed)
		{
			t.dirty = true;
			RebuildTerrain(mesh);
		}
		return changed;
	}
	bool RaycastTerrain(const Mesh &mesh, const glm::vec3 &eye, const glm::vec3 &ray, glm::vec3 &hit)
	{
		if (!mesh.m_terrain || mesh.m_subMeshes.empty())
			return false;
		auto &t = *mesh.m_terrain;
		auto &bounds = mesh.m_subMeshes.front();
		float enter = 0, exit = 1e30f;
		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(ray[axis]) < 1e-10f)
			{
				if (eye[axis] < bounds.m_minimum[axis] || eye[axis] > bounds.m_maximum[axis])
					return false;
			}
			else
			{
				float a = (bounds.m_minimum[axis] - eye[axis]) / ray[axis],
					  b = (bounds.m_maximum[axis] - eye[axis]) / ray[axis];
				enter = std::max(enter, std::min(a, b));
				exit = std::min(exit, std::max(a, b));
				if (enter > exit)
					return false;
			}
		}
		float dx = t.sizeX / (t.resX - 1), dz = t.sizeZ / (t.resZ - 1);
		auto point = eye + ray * enter;
		int x = glm::clamp(int(std::floor(point.x / dx)), 0, int(t.resX) - 2),
			z = glm::clamp(int(std::floor(point.z / dz)), 0, int(t.resZ) - 2);
		int stepX = ray.x > 0 ? 1 : ray.x < 0 ? -1 : 0, stepZ = ray.z > 0 ? 1 : ray.z < 0 ? -1 : 0;
		float nextX = stepX ? ((x + (stepX > 0 ? 1 : 0)) * dx - eye.x) / ray.x : 1e30f;
		float nextZ = stepZ ? ((z + (stepZ > 0 ? 1 : 0)) * dz - eye.z) / ray.z : 1e30f;
		float deltaX = stepX ? std::abs(dx / ray.x) : 1e30f, deltaZ = stepZ ? std::abs(dz / ray.z) : 1e30f;
		// 第四步：沿XZ网格逐格前进，只和射线经过的真实三角形求交，山地无需全网格遍历。
		while (x >= 0 && z >= 0 && x < int(t.resX) - 1 && z < int(t.resZ) - 1)
		{
			float closest = 1e30f;
			size_t first = (size_t(z) * (t.resX - 1) + x) * 6;
			for (size_t i = first; i < first + 6; i += 3)
			{
				auto a = mesh.m_vertices[mesh.m_indices[i]]._position,
					 b = mesh.m_vertices[mesh.m_indices[i + 1]]._position,
					 c = mesh.m_vertices[mesh.m_indices[i + 2]]._position;
				auto e1 = b - a, e2 = c - a, p = glm::cross(ray, e2);
				float det = glm::dot(e1, p);
				if (std::abs(det) < 1e-10f)
					continue;
				auto v = eye - a;
				float u = glm::dot(v, p) / det;
				if (u < -.00001f || u > 1.00001f)
					continue;
				auto q = glm::cross(v, e1);
				float w = glm::dot(ray, q) / det;
				if (w < -.00001f || u + w > 1.00001f)
					continue;
				float distance = glm::dot(e2, q) / det;
				if (distance >= 0 && distance < closest)
					closest = distance;
			}
			float boundary = std::min(nextX, nextZ);
			if (closest <= std::min(boundary, exit) + .0001f)
			{
				hit = eye + ray * closest;
				return true;
			}
			if (boundary >= exit || (!stepX && !stepZ))
				break;
			bool crossX = nextX <= nextZ, crossZ = nextZ <= nextX;
			if (crossX)
			{
				x += stepX;
				nextX += deltaX;
			}
			if (crossZ)
			{
				z += stepZ;
				nextZ += deltaZ;
			}
		}
		return false;
	}
} // namespace Rendering
