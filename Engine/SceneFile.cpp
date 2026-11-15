#include "SceneFile.h"
#include "Rendering/Terrain.h"
#include <functional>
#include <unordered_set>
#include "Rendering/MeshRenderer.h"
#include "Rendering/MeshManager.h"
#include "Rendering/TextureManager.h"
#include "Rendering/MaterialManager.h"
#include "Rendering/Lighting.h"
#include "Platform/Paths.h"
#include <lua.hpp>
#include <windows.h>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <locale>
#include <limits>
#include <algorithm>
#include <unordered_map>
namespace MyEngine
{
	namespace
	{
		constexpr size_t MaxFileSize = 64 * 1024 * 1024, MaxLuaMemory = 256 * 1024 * 1024;
		constexpr unsigned MaxNodes = 65536, MaxDepth = 64;
		[[noreturn]] void Fail(const std::string &i_message)
		{
			throw std::runtime_error("Scene file: " + i_message);
		}
		// 第一步：Lua 不开放标准库；分配器与指令钩子分别限制内存和死循环。
		struct LuaBudget
		{
			size_t used = 0;
			size_t limit = MaxLuaMemory;
			int instructions = 32000;
		};
		void *Allocate(void *i_user, void *i_pointer, size_t i_oldSize, size_t i_newSize)
		{
			auto &budget = *static_cast<LuaBudget *>(i_user);
			if (!i_pointer)
				i_oldSize = 0;
			if (!i_newSize)
			{
				std::free(i_pointer);
				budget.used -= i_oldSize;
				return nullptr;
			}
			if (i_newSize > budget.limit || budget.used - i_oldSize > budget.limit - i_newSize)
				return nullptr;
			auto result = std::realloc(i_pointer, i_newSize);
			if (result)
				budget.used = budget.used - i_oldSize + i_newSize;
			return result;
		}
		void InstructionHook(lua_State *i_state, lua_Debug *)
		{
			void *user = nullptr;
			lua_getallocf(i_state, &user);
			if (--static_cast<LuaBudget *>(user)->instructions <= 0)
				luaL_error(i_state, "scene execution budget exceeded");
		}
		void Table(lua_State *i_state, int i_index)
		{
			if (!lua_istable(i_state, i_index))
				Fail("expected a table");
		}
		size_t Array(lua_State *i_state, int i_index)
		{
			// 数组必须连续，拒绝字符串键和空洞，防止未知组件藏在非数组字段里被略过。
			Table(i_state, i_index);
			i_index = lua_absindex(i_state, i_index);
			auto length = lua_rawlen(i_state, i_index);
			size_t count = 0;
			lua_pushnil(i_state);
			while (lua_next(i_state, i_index))
			{
				if (!lua_isinteger(i_state, -2) || lua_tointeger(i_state, -2) < 1 ||
					static_cast<lua_Unsigned>(lua_tointeger(i_state, -2)) > length)
					Fail("expected a dense array without named fields");
				++count;
				lua_pop(i_state, 1);
			}
			if (count != length)
				Fail("array contains holes");
			return length;
		}
		void Field(lua_State *i_state, int i_index, const char *i_key)
		{
			// 原始读取不执行元表，文件中的 __index 无法劫持 C++ 验证流程。
			i_index = lua_absindex(i_state, i_index);
			lua_pushstring(i_state, i_key);
			lua_rawget(i_state, i_index);
		}
		float Number(lua_State *i_state, int i_index)
		{
			if (lua_type(i_state, i_index) != LUA_TNUMBER)
				Fail("expected a finite number");
			auto value = lua_tonumber(i_state, i_index);
			if (!std::isfinite(value) || std::abs(value) > 1e8)
				Fail("number outside supported range");
			return static_cast<float>(value);
		}
		float NumberField(lua_State *i_state, int i_table, const char *i_key)
		{
			Field(i_state, i_table, i_key);
			auto value = Number(i_state, -1);
			lua_pop(i_state, 1);
			return value;
		}
		std::string String(lua_State *i_state, int i_index)
		{
			if (lua_type(i_state, i_index) != LUA_TSTRING)
				Fail("expected UTF-8 string");
			size_t length = 0;
			auto text = lua_tolstring(i_state, i_index, &length);
			if (length > 4096)
				Fail("string too long");
			std::string result(text, length);
			if (result.find('\0') != std::string::npos)
				Fail("string contains NUL");
			Platform::ToWide(result);
			return result;
		}
		std::string StringField(lua_State *i_state, int i_table, const char *i_key)
		{
			Field(i_state, i_table, i_key);
			auto result = String(i_state, -1);
			lua_pop(i_state, 1);
			return result;
		}
		bool BoolField(lua_State *i_state, int i_table, const char *i_key)
		{
			Field(i_state, i_table, i_key);
			if (!lua_isboolean(i_state, -1))
				Fail(std::string("expected boolean: ") + i_key);
			bool value = lua_toboolean(i_state, -1) != 0;
			lua_pop(i_state, 1);
			return value;
		}
		glm::vec4 Vector(lua_State *i_state, int i_table, const char *i_key, int i_count)
		{
			Field(i_state, i_table, i_key);
			Table(i_state, -1);
			if (Array(i_state, -1) != i_count)
				Fail(std::string("wrong vector size: ") + i_key);
			glm::vec4 value(0);
			for (int i = 0; i < i_count; ++i)
			{
				lua_rawgeti(i_state, -1, i + 1);
				value[i] = Number(i_state, -1);
				lua_pop(i_state, 1);
			}
			lua_pop(i_state, 1);
			return value;
		}
		std::string Asset(const std::string &i_path)
		{
			// 第二步：只接受 Assets 内文件。canonical 同时拦住 .. 与目录链接逃逸。
			Platform::ToWide(i_path);
			auto relative = std::filesystem::u8path(i_path);
			if (i_path.empty() || relative.is_absolute() || relative.has_root_name() ||
				i_path.find(':') != std::string::npos || i_path.find('\0') != std::string::npos)
				Fail("resource must be relative to Assets: " + i_path);
			auto root = std::filesystem::canonical(Platform::GetAssetPath(""));
			auto full = std::filesystem::weakly_canonical(root / relative);
			auto local = full.lexically_relative(root);
			if (local.empty() || *local.begin() == ".." || !std::filesystem::is_regular_file(full))
				Fail("missing resource or path outside Assets: " + i_path);
			return local.generic_u8string();
		}
		Rendering::TexturePtr Texture(lua_State *i_state, int i_table, const char *i_key)
		{
			auto path = StringField(i_state, i_table, i_key);
			if (path.empty())
				return nullptr;
			path = Asset(path);
			return Rendering::TextureManager::GetInstance()->Load(path, path);
		}
		void ValidateModelResources(const Rendering::Mesh &i_mesh)
		{
			// 第一步：宽容导入留下的真实缺失/不支持诊断，在场景存档闭环中必须显式拒绝。
			if (!i_mesh.m_importIssues.empty())
				Fail("model cannot be saved or loaded losslessly: " + i_mesh.GetFileName() + ": " +
					 i_mesh.m_importIssues.front());
			// 第二步：检查成功读取的伴随文件及 OBJ 声明的材质库，覆盖绝对路径与目录链接。
			auto root = std::filesystem::canonical(Platform::GetAssetPath(""));
			for (const auto &dependency : i_mesh.m_importDependencies)
			{
				auto full = std::filesystem::weakly_canonical(dependency);
				auto relative = full.lexically_relative(root);
				if (relative.empty() || relative.is_absolute() || *relative.begin() == "..")
					Fail("model dependency is outside Assets: " + dependency.u8string());
				Asset(relative.generic_u8string());
			}
			// 第三步：模型内默认材质仍由导入器提供，其贴图必须能从工程资源恢复。
			for (const auto &material : i_mesh.m_materials)
			{
				if (!material)
					Fail("model contains a null default material");
				for (const auto &texture : {material->m_texture, material->m_normalTexture, material->m_opacityTexture})
					if (texture)
						Asset(texture->GetFileName());
			}
		}
		void ReadMaterial(lua_State *i_state, int i_table, Rendering::Material &o_material)
		{
			o_material.m_color = Vector(i_state, i_table, "color", 4);
			o_material.m_metallic = NumberField(i_state, i_table, "metallic");
			o_material.m_roughness = NumberField(i_state, i_table, "roughness");
			o_material.m_ao = NumberField(i_state, i_table, "ao");
			o_material.m_alphaCutoff = NumberField(i_state, i_table, "alphaCutoff");
			for (float v : {o_material.m_metallic, o_material.m_roughness, o_material.m_ao, o_material.m_alphaCutoff,
							o_material.m_color.a})
				if (v < 0 || v > 1)
					Fail("material parameter must be in [0,1]");
			o_material.m_transparent = BoolField(i_state, i_table, "transparent");
			o_material.m_texture = Texture(i_state, i_table, "texture");
			o_material.m_normalTexture = Texture(i_state, i_table, "normalTexture");
			o_material.m_opacityTexture = Texture(i_state, i_table, "opacityTexture");
			o_material.m_vertexShader = Asset(StringField(i_state, i_table, "vertexShader"));
			o_material.m_fragmentShader = Asset(StringField(i_state, i_table, "fragmentShader"));
			o_material.m_dxShader = Asset(StringField(i_state, i_table, "dxShader"));
		}
		using MaterialFiles = std::unordered_map<std::string, Rendering::MaterialPtr>;
		Rendering::MaterialPtr ReadMaterialOverride(lua_State *i_state, int i_table, MaterialFiles &io_files)
		{
			i_table = lua_absindex(i_state, i_table);
			Field(i_state, i_table, "file");
			bool fileReference = !lua_isnil(i_state, -1);
			lua_pop(i_state, 1);
			if (!fileReference)
			{
				static const std::unordered_set<std::string> fields = {
					"color",   "metallic",		"roughness",	  "ao",			  "alphaCutoff",	"transparent",
					"texture", "normalTexture", "opacityTexture", "vertexShader", "fragmentShader", "dxShader"};
				lua_pushnil(i_state);
				while (lua_next(i_state, i_table))
				{
					if (lua_type(i_state, -2) != LUA_TSTRING || !fields.count(String(i_state, -2)))
						Fail("unknown inline material field");
					lua_pop(i_state, 1);
				}
				auto material = std::make_shared<Rendering::Material>();
				ReadMaterial(i_state, i_table, *material);
				return material;
			}
			// 文件引用不能混入覆盖字段，避免缓存材质共享时悄悄丢弃内联值。
			lua_pushnil(i_state);
			while (lua_next(i_state, i_table))
			{
				if (lua_type(i_state, -2) != LUA_TSTRING || String(i_state, -2) != "file")
					Fail("material file reference cannot contain inline fields");
				lua_pop(i_state, 1);
			}
			auto path = StringField(i_state, i_table, "file");
			auto found = io_files.find(path);
			if (found != io_files.end())
				return found->second;
			auto local = Asset(path);
			auto material = Rendering::MaterialManager::GetInstance()->Load(local, local);
			// 缓存可能早于本次读取创建，仍验证当前材质的实际依赖。
			for (const auto &texture : {material->m_texture, material->m_normalTexture, material->m_opacityTexture})
				if (texture)
					Asset(texture->GetFileName());
			Asset(material->m_vertexShader);
			Asset(material->m_fragmentShader);
			Asset(material->m_dxShader);
			io_files.emplace(path, material);
			return material;
		}
		void ReadNode(lua_State *i_state, int i_table, Rendering::SceneNode &o_node, unsigned i_depth,
					  unsigned &o_count, bool i_validation, MaterialFiles &io_materialFiles)
		{
			if (i_depth > MaxDepth || ++o_count > MaxNodes)
				Fail("too many nodes or nesting too deep");
			if (!lua_checkstack(i_state, 16))
				Fail("Lua stack limit exceeded");
			i_table = lua_absindex(i_state, i_table);
			Table(i_state, i_table);
			o_node.SetNodeName(StringField(i_state, i_table, "name"));
			o_node.m_position = Rendering::FromGLM(glm::vec3(Vector(i_state, i_table, "position", 3)));
			o_node.m_scale = glm::vec3(Vector(i_state, i_table, "scale", 3));
			if (glm::any(glm::lessThan(glm::abs(o_node.m_scale), glm::vec3(.000001f))))
				Fail("scale must be nonzero for normal transformation");
			auto q = Vector(i_state, i_table, "rotation", 4);
			if (glm::length(q) < .00001f)
				Fail("zero rotation quaternion");
			o_node.m_orientation = glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
			Field(i_state, i_table, "components");
			Table(i_state, -1);
			size_t count = Array(i_state, -1);
			if (count > 3)
				Fail("too many components on a node");
			bool meshSeen = false, directionalSeen = false, pointSeen = false;
			for (size_t i = 1; i <= count; ++i)
			{
				lua_rawgeti(i_state, -1, i);
				int component = lua_gettop(i_state);
				Table(i_state, component);
				auto type = StringField(i_state, component, "type");
				if (type == "mesh")
				{
					if (meshSeen)
						Fail("duplicate mesh component");
					meshSeen = true;
					auto path = StringField(i_state, component, "model");
					Rendering::MeshPtr mesh;
					if (path.find("builtin:") == 0)
						mesh = Rendering::MeshManager::GetInstance()->GetBuiltin(path);
					else
					{
						path = Asset(path);
						// 保存后的回读只临时解码地形，避免每次快照被全局资源缓存永久持有。
						if (i_validation && std::filesystem::u8path(path).extension() == ".terrain")
							mesh = std::make_shared<Rendering::Mesh>(path);
						else
							mesh = Rendering::MeshManager::GetInstance()->Load(path, path);
						ValidateModelResources(*mesh);
					}
					auto renderer = o_node.AddComponent<Rendering::MeshRenderer>(mesh);
					Field(i_state, component, "materials");
					if (!lua_isnil(i_state, -1))
					{
						Table(i_state, -1);
						auto materials = Array(i_state, -1);
						if (materials > mesh->GetSubmeshCount())
							Fail("too many material overrides");
						for (size_t m = 1; m <= materials; ++m)
						{
							lua_rawgeti(i_state, -1, m);
							Table(i_state, -1);
							auto material = ReadMaterialOverride(i_state, lua_gettop(i_state), io_materialFiles);
							renderer->AddMaterial(material);
							lua_pop(i_state, 1);
						}
					}
					lua_pop(i_state, 1);
				}
				else if (type == "directional" || type == "point")
				{
					Rendering::LightBase *light;
					if (type == "directional")
					{
						if (directionalSeen)
							Fail("duplicate directional light");
						directionalSeen = true;
						light = o_node.AddComponent<Rendering::DirectionalLight>();
					}
					else
					{
						if (pointSeen)
							Fail("duplicate point light");
						pointSeen = true;
						light = o_node.AddComponent<Rendering::PointLight>();
					}
					light->m_diffuse = glm::vec3(Vector(i_state, component, "color", 3));
					light->m_direction = glm::vec3(Vector(i_state, component, "direction", 3));
					light->m_intensity = NumberField(i_state, component, "intensity");
					auto attenuation = Vector(i_state, component, "attenuation", 3);
					light->m_constant = attenuation.x;
					light->m_linear = attenuation.y;
					light->m_quadratic = attenuation.z;
					if (light->m_intensity < 0 || glm::any(glm::lessThan(attenuation, glm::vec4(0))) ||
						glm::length(light->m_direction) < .00001f || glm::length(glm::vec3(attenuation)) < .00001f)
						Fail("invalid light direction, intensity or attenuation");
				}
				else
					Fail("unsupported component: " + type);
				lua_pop(i_state, 1);
			}
			lua_pop(i_state, 1);
			Field(i_state, i_table, "children");
			Table(i_state, -1);
			auto children = Array(i_state, -1);
			if (children > MaxNodes)
				Fail("too many children");
			for (size_t i = 1; i <= children; ++i)
			{
				lua_rawgeti(i_state, -1, i);
				auto child = std::make_unique<Rendering::SceneNode>();
				ReadNode(i_state, lua_gettop(i_state), *child, i_depth + 1, o_count, i_validation, io_materialFiles);
				o_node.AddChild(child.get());
				child.release();
				lua_pop(i_state, 1);
			}
			lua_pop(i_state, 1);
		}
		SceneFileData Parse(const std::string &i_text, bool i_validation = false)
		{
			LuaBudget budget;
			std::unique_ptr<lua_State, decltype(&lua_close)> state(lua_newstate(Allocate, &budget), lua_close);
			auto s = state.get();
			if (!s)
				throw std::bad_alloc();
			lua_sethook(s, InstructionHook, LUA_MASKCOUNT, 1000);
			if (luaL_loadbufferx(s, i_text.data(), i_text.size(), "scene", "t") || lua_pcall(s, 0, 1, 0))
				Fail(lua_tostring(s, -1) ? lua_tostring(s, -1) : "Lua parse failed");
			// 文件执行结束后保留额外的小块空间给原始字段读取和 C 栈扩展。
			budget.limit += 4 * 1024 * 1024;
			Table(s, -1);
			int root = lua_gettop(s);
			if (NumberField(s, root, "version") != 1)
				Fail("unsupported version");
			SceneFileData result;
			auto &settings = result.settings;
			Field(s, root, "camera");
			Table(s, -1);
			int camera = lua_gettop(s);
			settings.cameraEye = glm::vec3(Vector(s, camera, "eye", 3));
			settings.cameraDirection = glm::vec3(Vector(s, camera, "direction", 3));
			settings.cameraFov = NumberField(s, camera, "fov");
			settings.cameraNear = NumberField(s, camera, "near");
			settings.cameraFar = NumberField(s, camera, "far");
			if (settings.cameraFov <= 1 || settings.cameraFov >= 179 || settings.cameraNear <= 0 ||
				settings.cameraFar <= settings.cameraNear || glm::length(settings.cameraDirection) < .00001f ||
				glm::length(glm::cross(settings.cameraDirection, glm::vec3(0, 1, 0))) < .00001f)
				Fail("invalid camera projection or direction");
			lua_pop(s, 1);
			Field(s, root, "sky");
			Table(s, -1);
			settings.drawSky = BoolField(s, lua_gettop(s), "enabled");
			Field(s, -1, "faces");
			Table(s, -1);
			if (Array(s, -1) != 6)
				Fail("sky requires six faces");
			for (int i = 0; i < 6; ++i)
			{
				lua_rawgeti(s, -1, i + 1);
				settings.skyFaces[i] = Asset(String(s, -1));
				lua_pop(s, 1);
			}
			lua_pop(s, 1);
			Field(s, -1, "ambientFaces");
			if (!lua_isnil(s, -1))
			{
				if (Array(s, -1) != 6)
					Fail("ambientFaces requires six faces");
				for (int i = 0; i < 6; ++i)
				{
					lua_rawgeti(s, -1, i + 1);
					settings.ambientFaces[i] = Asset(String(s, -1));
					lua_pop(s, 1);
				}
			}
			lua_pop(s, 2);
			// 第三步：所有 Actor 先建在新 Scene 内；异常销毁临时结果，不动旧场景。
			result.scene = std::make_unique<Scene>();
			Field(s, root, "actors");
			Table(s, -1);
			auto actors = Array(s, -1);
			if (actors > MaxNodes)
				Fail("too many actors");
			unsigned nodes = 0;
			// 单次场景只校验每个引用一次，大量实例共享全局材质和贴图缓存。
			MaterialFiles materialFiles;
			for (size_t i = 1; i <= actors; ++i)
			{
				lua_rawgeti(s, -1, i);
				ActorPtr actor(new Actor());
				ReadNode(s, lua_gettop(s), *actor, 0, nodes, i_validation, materialFiles);
				result.scene->addActor(actor);
				lua_pop(s, 1);
			}
			return result;
		}
		std::string Quote(const std::string &i_text)
		{
			if (i_text.size() > 4096 || i_text.find('\0') != std::string::npos)
				Fail("invalid string");
			Platform::ToWide(i_text);
			std::string result = "\"";
			for (unsigned char c : i_text)
			{
				if (c == '\\' || c == '"')
				{
					result += '\\';
					result += c;
				}
				else if (c < 32 || c == 127)
				{
					result += '\\';
					result += char('0' + c / 100);
					result += char('0' + c / 10 % 10);
					result += char('0' + c % 10);
				}
				else
					result += c;
			}
			return result + "\"";
		}
		void WriteVector(std::ostream &o_stream, const glm::vec4 &i_value, int i_count)
		{
			o_stream << '{';
			for (int i = 0; i < i_count; ++i)
			{
				if (!std::isfinite(i_value[i]) || std::abs(i_value[i]) > 1e8f)
					Fail("invalid numeric value on save");
				if (i)
					o_stream << ',';
				o_stream << i_value[i];
			}
			o_stream << '}';
		}
		void WriteMaterial(std::ostream &o_stream, const Rendering::Material &i_material)
		{
			o_stream << "{color=";
			WriteVector(o_stream, i_material.m_color, 4);
			o_stream << ",metallic=" << i_material.m_metallic << ",roughness=" << i_material.m_roughness
					 << ",ao=" << i_material.m_ao << ",alphaCutoff=" << i_material.m_alphaCutoff
					 << ",transparent=" << (i_material.m_transparent ? "true" : "false");
			auto texture = [&](const char *key, const Rendering::TexturePtr &value)
			{ o_stream << ',' << key << '=' << Quote(value ? Asset(value->GetFileName()) : ""); };
			texture("texture", i_material.m_texture);
			texture("normalTexture", i_material.m_normalTexture);
			texture("opacityTexture", i_material.m_opacityTexture);
			o_stream << ",vertexShader=" << Quote(Asset(i_material.m_vertexShader))
					 << ",fragmentShader=" << Quote(Asset(i_material.m_fragmentShader))
					 << ",dxShader=" << Quote(Asset(i_material.m_dxShader)) << '}';
		}
		void WriteNode(std::ostream &o_stream, const Rendering::SceneNode &i_node, unsigned i_depth, unsigned &o_count)
		{
			if (i_depth > MaxDepth || ++o_count > MaxNodes)
				Fail("too many nodes or nesting too deep");
			o_stream << "{name=" << Quote(i_node.GetNodeName()) << ",position=";
			WriteVector(o_stream, glm::vec4(Rendering::ToGLM(i_node.m_position), 0), 3);
			o_stream << ",scale=";
			WriteVector(o_stream, glm::vec4(i_node.m_scale, 0), 3);
			auto q = i_node.m_orientation;
			o_stream << ",rotation=";
			WriteVector(o_stream, glm::vec4(q.x, q.y, q.z, q.w), 4);
			o_stream << ",components={\n";
			for (const auto &entry : i_node.GetComponents())
			{
				if (entry.first == std::type_index(typeid(Rendering::MeshRenderer)))
				{
					auto renderer = static_cast<Rendering::MeshRenderer *>(entry.second.get());
					if (!renderer->GetMesh())
						Fail("null mesh");
					ValidateModelResources(*renderer->GetMesh());
					auto path = renderer->GetMesh()->GetFileName();
					const auto &builtins = Rendering::Mesh::GetBuiltinNames();
					if (std::find(builtins.begin(), builtins.end(), path) == builtins.end())
						path = Asset(path);
					o_stream << "{type=\"mesh\",model=" << Quote(path);
					if (!renderer->GetMaterials().empty())
					{
						o_stream << ",materials={";
						for (const auto &material : renderer->GetMaterials())
						{
							if (!material)
								Fail("null material override is unsupported");
							WriteMaterial(o_stream, *material);
							o_stream << ',';
						}
						o_stream << '}';
					}
					o_stream << "},\n";
				}
				else if (entry.first == std::type_index(typeid(Rendering::DirectionalLight)) ||
						 entry.first == std::type_index(typeid(Rendering::PointLight)))
				{
					auto light = static_cast<Rendering::LightBase *>(entry.second.get());
					o_stream << "{type=" << Quote(light->GetType() == Rendering::DIRECTIONAL ? "directional" : "point")
							 << ",color=";
					WriteVector(o_stream, glm::vec4(light->m_diffuse, 0), 3);
					o_stream << ",direction=";
					WriteVector(o_stream, glm::vec4(light->m_direction, 0), 3);
					o_stream << ",intensity=" << light->m_intensity << ",attenuation=";
					WriteVector(o_stream, glm::vec4(light->m_constant, light->m_linear, light->m_quadratic, 0), 3);
					o_stream << "},\n";
				}
				else
					Fail("unsupported component on node " + i_node.GetNodeName() + ": " + entry.first.name());
			}
			o_stream << "},children={\n";
			for (const auto &child : i_node.GetChildren())
			{
				WriteNode(o_stream, *child, i_depth + 1, o_count);
				o_stream << ",\n";
			}
			o_stream << "}}";
		}
	} // namespace
	SceneFileData LoadSceneFile(const std::filesystem::path &i_path)
	{
		// 先限制磁盘大小再读入，避免损坏文件触发无界分配。
		std::ifstream file(i_path, std::ios::binary | std::ios::ate);
		if (!file)
			Fail("cannot open " + i_path.u8string());
		auto length = file.tellg();
		if (length < 0 || length > static_cast<std::streamoff>(MaxFileSize))
			Fail("file is too large");
		std::string text(static_cast<size_t>(length), '\0');
		file.seekg(0);
		if (length && !file.read(text.data(), length))
			Fail("incomplete read");
		// UTF-8 BOM 常见于 Windows 编辑器；loadbuffer 不像 loadfile 自动跳过它。
		if (text.compare(0, 3, "\xEF\xBB\xBF") == 0)
			text.erase(0, 3);
		return Parse(text);
	}
	void SaveSceneFile(const Scene &i_scene, const std::filesystem::path &i_path, const SceneFileSettings &i_settings)
	{
		// 地形保存成不可变快照：后续笔刷和另存为不会改动源资源或任何旧场景引用的高度。
		std::unordered_set<uint64_t> saved;
		std::vector<Rendering::MeshPtr> terrainSnapshots;
		std::function<void(const Rendering::SceneNode &)> saveTerrain = [&](const Rendering::SceneNode &node)
		{
			if (auto renderer = node.GetComponent<Rendering::MeshRenderer>())
			{
				auto mesh = renderer->GetMesh();
				if (mesh->m_terrain && mesh->m_terrain->dirty && saved.insert(mesh->GetHandle()).second)
				{
					auto name = "Terrains/Edits/terrain-" + std::to_string(GetCurrentProcessId()) + "-" +
								std::to_string(GetTickCount64()) + "-" + std::to_string(mesh->GetHandle()) + "-" +
								std::to_string(mesh->m_revision) + ".terrain";
					Rendering::SaveTerrain(*mesh, name);
					terrainSnapshots.push_back(mesh);
				}
			}
			for (const auto &child : node.GetChildren())
				saveTerrain(*child);
		};
		for (const auto &actor : i_scene.GetActors())
			saveTerrain(*actor);
		// 第一步：先在内存生成完整文本，拒绝未知组件与外部资源，绝不覆盖半份场景。
		std::ostringstream stream;
		stream.imbue(std::locale::classic());
		stream << std::setprecision(std::numeric_limits<float>::max_digits10);
		stream << "-- Renderer scene format 1; rotation = quaternion x,y,z,w\nreturn {version=1,camera={eye=";
		WriteVector(stream, glm::vec4(i_settings.cameraEye, 0), 3);
		stream << ",direction=";
		WriteVector(stream, glm::vec4(i_settings.cameraDirection, 0), 3);
		stream << ",fov=" << i_settings.cameraFov << ",near=" << i_settings.cameraNear
			   << ",far=" << i_settings.cameraFar << "},sky={enabled=" << (i_settings.drawSky ? "true" : "false")
			   << ",faces={";
		for (const auto &face : i_settings.skyFaces)
			stream << Quote(Asset(face)) << ',';
		stream << '}';
		if (std::any_of(i_settings.ambientFaces.begin(), i_settings.ambientFaces.end(),
						[](const auto &face) { return !face.empty(); }))
		{
			stream << ",ambientFaces={";
			for (const auto &face : i_settings.ambientFaces)
				stream << Quote(Asset(face)) << ',';
			stream << '}';
		}
		stream << "},actors={\n";
		unsigned nodes = 0;
		for (const auto &actor : i_scene.GetActors())
		{
			WriteNode(stream, *actor, 0, nodes);
			stream << ",\n";
		}
		stream << "}}\n";
		auto text = stream.str();
		if (text.size() > MaxFileSize)
			Fail("serialized scene is too large");
		// 第二步：用同一读取校验器核验数值及结构；失败时旧文件原样保留。
		Parse(text, true);
		if (i_path.has_parent_path())
			std::filesystem::create_directories(i_path.parent_path());
		auto temporary = i_path;
		temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
		try
		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			if (!file || !file.write(text.data(), static_cast<std::streamsize>(text.size())))
				Fail("cannot write temporary file");
			file.flush();
			if (!file)
				Fail("cannot flush temporary file");
			file.close();
			if (!file)
				Fail("cannot close temporary file");
			// 第三步：同目录替换，Windows 支持覆盖旧文件；失败不先删除旧文件。
			if (!MoveFileExW(temporary.c_str(), i_path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				Fail("cannot replace destination file (Windows error " + std::to_string(GetLastError()) + ")");
			for (auto &mesh : terrainSnapshots)
				mesh->m_terrain->dirty = false;
		}
		catch (...)
		{
			std::error_code error;
			std::filesystem::remove(temporary, error);
			throw;
		}
	}
} // namespace MyEngine
