#include "Mesh.h"
#include <cmath>
#include <stdexcept>

namespace Rendering
{
	namespace
	{
		constexpr float Pi = 3.14159265359f;
		constexpr unsigned Segments = 32;
		struct ProfilePoint
		{
			float radius, height, normalRadius, normalHeight, v;
		};
		void AddRevolvedSurface(const std::vector<ProfilePoint> &i_profile, std::vector<Vertex> &o_vertices,
								std::vector<unsigned> &o_indices)
		{
			// 把竖直截面绕 Y 轴转一圈，每层生成一个圆环，UV 接缝处保留两个顶点。
			unsigned offset = static_cast<unsigned>(o_vertices.size());
			for (const auto &point : i_profile)
				for (unsigned side = 0; side <= Segments; ++side)
				{
					float u = float(side) / Segments, angle = u * 2 * Pi;
					float x = std::cos(angle), z = std::sin(angle);
					o_vertices.push_back(
						{{point.radius * x, point.height, point.radius * z},
						 glm::normalize(glm::vec3(point.normalRadius * x, point.normalHeight, point.normalRadius * z)),
						 {u, point.v}});
				}
			// 相邻两圈之间拼两个三角形，绕序朝外。球极点/锥顶处跳过面积为零的三角形。
			for (unsigned row = 0; row + 1 < i_profile.size(); ++row)
				for (unsigned side = 0; side < Segments; ++side)
				{
					unsigned a = offset + row * (Segments + 1) + side, b = a + Segments + 1;
					if (i_profile[row].radius > .000001f)
						o_indices.insert(o_indices.end(), {a, a + 1, b});
					if (i_profile[row + 1].radius > .000001f)
						o_indices.insert(o_indices.end(), {a + 1, b + 1, b});
				}
		}
		void AddCap(float i_height, float i_radius, bool i_top, std::vector<Vertex> &o_vertices,
					std::vector<unsigned> &o_indices)
		{
			// 端盖独立使用一圈顶点，法线朝上/下，不与侧面的光滑法线混在一起。
			unsigned center = static_cast<unsigned>(o_vertices.size());
			glm::vec3 normal(0, i_top ? 1.f : -1.f, 0);
			o_vertices.push_back({{0, i_height, 0}, normal, {.5f, .5f}});
			for (unsigned side = 0; side <= Segments; ++side)
			{
				float angle = float(side) / Segments * 2 * Pi;
				float x = std::cos(angle), z = std::sin(angle);
				o_vertices.push_back({{x * i_radius, i_height, z * i_radius}, normal, {x * .5f + .5f, z * .5f + .5f}});
			}
			for (unsigned side = 0; side < Segments; ++side)
			{
				unsigned a = center + 1 + side, b = a + 1;
				if (i_top)
					o_indices.insert(o_indices.end(), {center, b, a});
				else
					o_indices.insert(o_indices.end(), {center, a, b});
			}
		}
	} // namespace
	const std::vector<std::string> &Mesh::GetBuiltinNames()
	{
		static const std::vector<std::string> names = {"builtin:cube",	  "builtin:quad",	  "builtin:plane",
													   "builtin:sphere",  "builtin:cylinder", "builtin:cone",
													   "builtin:capsule", "builtin:torus"};
		return names;
	}
	MeshPtr Mesh::CreateBuiltin(const std::string &i_name)
	{
		if (i_name == "builtin:cube")
			return CreateCube();
		if (i_name == "builtin:quad")
			return CreateQuad();
		std::vector<Vertex> vertices;
		std::vector<unsigned> indices;
		std::vector<ProfilePoint> profile;
		if (i_name == "builtin:plane")
		{
			// 与原 Quad 的竖直 XY 平面分开：Plane 放在水平 XZ 平面，朝上，用来搭地面。
			vertices = {{{-.5f, 0, -.5f}, {0, 1, 0}, {0, 0}},
						{{-.5f, 0, .5f}, {0, 1, 0}, {0, 1}},
						{{.5f, 0, .5f}, {0, 1, 0}, {1, 1}},
						{{.5f, 0, -.5f}, {0, 1, 0}, {1, 0}}};
			indices = {0, 1, 2, 0, 2, 3};
		}
		else if (i_name == "builtin:sphere")
		{
			// 纬度从北极走到南极。单位法线就是球心指向表面的方向，半径取 0.5。
			for (unsigned row = 0; row <= 24; ++row)
			{
				float v = float(row) / 24, angle = v * Pi;
				profile.push_back({.5f * std::sin(angle), .5f * std::cos(angle), std::sin(angle), std::cos(angle), v});
			}
		}
		else if (i_name == "builtin:cylinder" || i_name == "builtin:cone")
		{
			bool cone = i_name == "builtin:cone";
			// 圆锥侧面斜率为 半径/高度=0.5；圆柱侧面法线则完全水平。
			profile = {{cone ? 0.f : .5f, .5f, 1, cone ? .5f : 0.f, 0}, {.5f, -.5f, 1, cone ? .5f : 0.f, 1}};
			AddCap(-.5f, .5f, false, vertices, indices);
			if (!cone)
				AddCap(.5f, .5f, true, vertices, indices);
		}
		else if (i_name == "builtin:capsule")
		{
			// 上下两个半球之间接一段直筒：半径0.25、总高1，接缝处位置和法线连续。
			float radius = .25f, totalLength = Pi * radius + .5f;
			for (unsigned half = 0; half < 2; ++half)
				for (unsigned row = 0; row <= 12; ++row)
				{
					float angle = (float(half) + float(row) / 12) * Pi * .5f;
					float height = (half == 0 ? .25f : -.25f) + radius * std::cos(angle);
					float v = (radius * angle + (half == 0 ? 0.f : .5f)) / totalLength;
					profile.push_back({radius * std::sin(angle), height, std::sin(angle), std::cos(angle), v});
				}
		}
		else if (i_name == "builtin:torus")
		{
			// 大圆半径0.35，小圆半径0.15；两个角分别沿圆环和管壁走一圈。
			constexpr unsigned tubeSegments = 16;
			for (unsigned ring = 0; ring <= Segments; ++ring)
				for (unsigned tube = 0; tube <= tubeSegments; ++tube)
				{
					float u = float(ring) / Segments, v = float(tube) / tubeSegments;
					float a = u * 2 * Pi, b = v * 2 * Pi, radius = .35f + .15f * std::cos(b);
					vertices.push_back({{radius * std::cos(a), .15f * std::sin(b), radius * std::sin(a)},
										{std::cos(b) * std::cos(a), std::sin(b), std::cos(b) * std::sin(a)},
										{u, v}});
				}
			for (unsigned ring = 0; ring < Segments; ++ring)
				for (unsigned tube = 0; tube < tubeSegments; ++tube)
				{
					unsigned a = ring * (tubeSegments + 1) + tube, b = a + tubeSegments + 1;
					indices.insert(indices.end(), {a, a + 1, b, a + 1, b + 1, b});
				}
		}
		else
			throw std::invalid_argument("Unknown built-in mesh: " + i_name);
		if (!profile.empty())
			AddRevolvedSurface(profile, vertices, indices);
		auto mesh = std::make_shared<Mesh>(vertices, indices);
		mesh->m_fileName = i_name;
		return mesh;
	}
} // namespace Rendering
