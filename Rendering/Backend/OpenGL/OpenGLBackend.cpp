#include "Rendering/Backend/IRenderBackend.h"
#include "Rendering/TextureManager.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include <windows.h>
#include <glew.h>
#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>
#include <map>
#include <stdexcept>
#include <algorithm>
namespace Rendering
{
	namespace GL
	{
#define GL_FUNCTIONS(F)                                                                                                \
	F(PFNGLCREATESHADERPROC, CreateShader)                                                                             \
	F(PFNGLSHADERSOURCEPROC, ShaderSource)                                                                             \
	F(PFNGLCOMPILESHADERPROC, CompileShader)                                                                           \
	F(PFNGLGETSHADERIVPROC, GetShaderiv)                                                                               \
	F(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)                                                                     \
	F(PFNGLDELETESHADERPROC, DeleteShader)                                                                             \
	F(PFNGLCREATEPROGRAMPROC, CreateProgram)                                                                           \
	F(PFNGLATTACHSHADERPROC, AttachShader) F(PFNGLLINKPROGRAMPROC, LinkProgram) F(PFNGLGETPROGRAMIVPROC, GetProgramiv) \
		F(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog) F(PFNGLDELETEPROGRAMPROC, DeleteProgram)                      \
			F(PFNGLUSEPROGRAMPROC, UseProgram) F(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation)                      \
				F(PFNGLUNIFORMMATRIX4FVPROC, UniformMatrix4fv) F(PFNGLUNIFORM4FVPROC, Uniform4fv)                      \
					F(PFNGLUNIFORM1IPROC, Uniform1i) F(PFNGLACTIVETEXTUREPROC, ActiveTexture)                          \
						F(PFNGLGENVERTEXARRAYSPROC, GenVertexArrays) F(PFNGLBINDVERTEXARRAYPROC, BindVertexArray)      \
							F(PFNGLDELETEVERTEXARRAYSPROC, DeleteVertexArrays) F(PFNGLGENBUFFERSPROC, GenBuffers)      \
								F(PFNGLBINDBUFFERPROC, BindBuffer) F(PFNGLBUFFERDATAPROC, BufferData)                  \
									F(PFNGLDELETEBUFFERSPROC, DeleteBuffers) F(PFNGLENABLEVERTEXATTRIBARRAYPROC,       \
																			   EnableVertexAttribArray)                \
										F(PFNGLVERTEXATTRIBPOINTERPROC,                                                \
										  VertexAttribPointer) F(PFNGLGENFRAMEBUFFERSPROC, GenFramebuffers)            \
											F(PFNGLBINDFRAMEBUFFERPROC,                                                \
											  BindFramebuffer) F(PFNGLDELETEFRAMEBUFFERSPROC, DeleteFramebuffers)      \
												F(PFNGLGENRENDERBUFFERSPROC,                                           \
												  GenRenderbuffers) F(PFNGLBINDRENDERBUFFERPROC, BindRenderbuffer)     \
													F(PFNGLDELETERENDERBUFFERSPROC,                                    \
													  DeleteRenderbuffers) F(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC,  \
																			 RenderbufferStorageMultisample)           \
														F(PFNGLFRAMEBUFFERRENDERBUFFERPROC, FramebufferRenderbuffer)   \
															F(PFNGLCHECKFRAMEBUFFERSTATUSPROC, CheckFramebufferStatus) \
																F(PFNGLBLITFRAMEBUFFERPROC, BlitFramebuffer)
#define DECLARE_GL(type, name) static type name = nullptr;
		GL_FUNCTIONS(DECLARE_GL)
		static PFNGLGENERATEMIPMAPPROC GenerateMipmap = nullptr;
		static PFNGLGENQUERIESPROC GenQueries = nullptr;
		static PFNGLDELETEQUERIESPROC DeleteQueries = nullptr;
		static PFNGLBEGINQUERYPROC BeginQuery = nullptr;
		static PFNGLENDQUERYPROC EndQuery = nullptr;
		static PFNGLGETQUERYOBJECTUI64VPROC GetQueryObjectui64v = nullptr;
		static PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D = nullptr;
#undef DECLARE_GL
		template <class T> T Load(const char *i_name)
		{
			auto pointer = wglGetProcAddress(i_name);
			if (!pointer || pointer == reinterpret_cast<PROC>(1) || pointer == reinterpret_cast<PROC>(2) ||
				pointer == reinterpret_cast<PROC>(3) || pointer == reinterpret_cast<PROC>(-1))
				throw std::runtime_error(std::string("OpenGL function missing: ") + i_name);
			return reinterpret_cast<T>(pointer);
		}
		void Initilize()
		{
			GenerateMipmap = Load<PFNGLGENERATEMIPMAPPROC>("glGenerateMipmap");
			GenQueries = Load<PFNGLGENQUERIESPROC>("glGenQueries");
			DeleteQueries = Load<PFNGLDELETEQUERIESPROC>("glDeleteQueries");
			BeginQuery = Load<PFNGLBEGINQUERYPROC>("glBeginQuery");
			EndQuery = Load<PFNGLENDQUERYPROC>("glEndQuery");
			GetQueryObjectui64v = Load<PFNGLGETQUERYOBJECTUI64VPROC>("glGetQueryObjectui64v");
			FramebufferTexture2D = Load<PFNGLFRAMEBUFFERTEXTURE2DPROC>("glFramebufferTexture2D");
#define LOAD_GL(type, name) name = Load<type>("gl" #name);
			GL_FUNCTIONS(LOAD_GL)
#undef LOAD_GL
		}
	} // namespace GL
	class OpenGLBackend final : public IRenderBackend
	{
	  public:
		~OpenGLBackend() override { ShutDown(); }
		void Initilize(void *i_pWindow, int i_width, int i_height, int i_samples, bool) override;
		void Resize(int i_width, int i_height) override;
		void Render(const RenderFrame &i_frame) override;
		void CaptureFrame(const std::filesystem::path &i_path) override;
		void Present(bool i_vsync) override;
		void ShutDown() override;
		void SetProfiling(bool i_enabled) override
		{
			m_profiling = i_enabled;
			if (i_enabled && !m_timeQuery)
				GL::GenQueries(1, &m_timeQuery);
		}
		double GetGPUTimeMilliseconds() const override { return m_gpuTime; }

	  private:
		struct Buffers
		{
			GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
			uint64_t revision = 0;
			bool terrain = false;
			std::weak_ptr<Mesh> owner;
		};
		HWND m_window = nullptr;
		HDC m_dc = nullptr;
		HGLRC m_context = nullptr;
		GLuint m_program = 0, m_framebuffer = 0, m_color = 0, m_depth = 0;
		GLuint m_shadowFramebuffer = 0, m_shadowTexture = 0, m_shadowDepth = 0;
		static constexpr int ShadowSize = 2048;
		int m_width = 0, m_height = 0, m_samples = 1;
		bool m_loaded = false;
		bool m_profiling = false;
		GLuint m_timeQuery = 0;
		double m_gpuTime = -1;
		using SwapIntervalProc = BOOL(WINAPI *)(int);
		SwapIntervalProc m_swapInterval = nullptr;
		std::map<uint64_t, Buffers> m_meshes;
		std::map<uint64_t, GLuint> m_textures, m_cubes;
		std::map<std::string, GLuint> m_programs;
		GLuint Compile(const std::string &i_source, GLenum i_type);
		GLuint GetProgram(const MaterialPtr &i_material);
		GLuint GetTexture(const TexturePtr &i_texture);
		GLuint GetCube(const TextureCubePtr &i_cube);
		Buffers &GetMesh(const MeshPtr &i_mesh);
		void Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode);
		void DeleteTargets();
	};
	void OpenGLBackend::Initilize(void *i_pWindow, int i_width, int i_height, int i_samples, bool)
	{
		m_window = static_cast<HWND>(i_pWindow);
		m_dc = GetDC(m_window);
		PIXELFORMATDESCRIPTOR format = {};
		format.nSize = sizeof(format);
		format.nVersion = 1;
		format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		format.iPixelType = PFD_TYPE_RGBA;
		format.cColorBits = 32;
		format.cDepthBits = 24;
		format.cStencilBits = 8;
		format.iLayerType = PFD_MAIN_PLANE;
		int selected = ChoosePixelFormat(m_dc, &format);
		if (!selected || !SetPixelFormat(m_dc, selected, &format))
			throw std::runtime_error("Cannot set OpenGL pixel format");
		m_context = wglCreateContext(m_dc);
		if (!m_context || !wglMakeCurrent(m_dc, m_context))
			throw std::runtime_error("Cannot create bootstrap OpenGL context");
		using CreateContextProc = HGLRC(WINAPI *)(HDC, HGLRC, const int *);
		auto createContext = GL::Load<CreateContextProc>("wglCreateContextAttribsARB");
		int attributes[] = {0x2091, 3, 0x2092, 3, 0x9126, 1, 0};
		HGLRC core = createContext(m_dc, nullptr, attributes);
		if (!core)
			throw std::runtime_error(
				"OpenGL 3.3 core is unavailable. Install a graphics driver or select --renderer dx11.");
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(m_context);
		m_context = core;
		if (!wglMakeCurrent(m_dc, m_context))
			throw std::runtime_error("Cannot activate OpenGL core context");
		GL::Initilize();
		m_loaded = true;
		m_swapInterval = reinterpret_cast<SwapIntervalProc>(wglGetProcAddress("wglSwapIntervalEXT"));
		m_program = GetProgram(nullptr);
		GLint maxSamples = 1;
		glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
		m_samples = 1;
		while (m_samples * 2 <= std::min(i_samples, maxSamples))
			m_samples *= 2;
		Resize(i_width, i_height);
		// 太阳阴影固定分辨率，不随窗口大小重建。
		GL::GenFramebuffers(1, &m_shadowFramebuffer);
		GL::BindFramebuffer(GL_FRAMEBUFFER, m_shadowFramebuffer);
		GL::ActiveTexture(GL_TEXTURE5);
		glGenTextures(1, &m_shadowTexture);
		glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, ShadowSize, ShadowSize, 0, GL_RED, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		const GLfloat border[] = {1, 1, 1, 1};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
		GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_shadowTexture, 0);
		GL::GenRenderbuffers(1, &m_shadowDepth);
		GL::BindRenderbuffer(GL_RENDERBUFFER, m_shadowDepth);
		GL::RenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24, ShadowSize, ShadowSize);
		GL::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_shadowDepth);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		if (GL::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error("OpenGL shadow framebuffer is incomplete");
		glBindTexture(GL_TEXTURE_2D, 0);
		GL::ActiveTexture(GL_TEXTURE0);
		GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
		Platform::Log(std::string("Renderer: OpenGL ") + reinterpret_cast<const char *>(glGetString(GL_VERSION)) +
					  ", GPU=" + reinterpret_cast<const char *>(glGetString(GL_RENDERER)) +
					  ", samples=" + std::to_string(m_samples));
	}
	GLuint OpenGLBackend::Compile(const std::string &i_source, GLenum i_type)
	{
		GLuint shader = GL::CreateShader(i_type);
		const char *pSource = i_source.c_str();
		GL::ShaderSource(shader, 1, &pSource, nullptr);
		GL::CompileShader(shader);
		GLint compiled = 0;
		GL::GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			GLint size = 0;
			GL::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
			std::string log(std::max(1, size), '\0');
			GL::GetShaderInfoLog(shader, size, nullptr, log.data());
			GL::DeleteShader(shader);
			throw std::runtime_error("GL shader compile failed: " + log);
		}
		return shader;
	}
	GLuint OpenGLBackend::GetProgram(const MaterialPtr &i_material)
	{
		std::string vs = i_material ? i_material->m_vertexShader : "Shaders/OpenGL/DefaultVS.shader";
		std::string fs = i_material ? i_material->m_fragmentShader : "Shaders/OpenGL/DefaultFS.shader";
		std::string key = vs + "\n" + fs;
		auto it = m_programs.find(key);
		if (it != m_programs.end())
			return it->second;
		GLuint vertex = Compile(Platform::ReadText(Platform::GetAssetPath(vs)), GL_VERTEX_SHADER), fragment = 0;
		try
		{
			fragment = Compile(Platform::ReadText(Platform::GetAssetPath(fs)), GL_FRAGMENT_SHADER);
		}
		catch (...)
		{
			GL::DeleteShader(vertex);
			throw;
		}
		GLuint program = GL::CreateProgram();
		GL::AttachShader(program, vertex);
		GL::AttachShader(program, fragment);
		GL::LinkProgram(program);
		GL::DeleteShader(vertex);
		GL::DeleteShader(fragment);
		GLint linked = 0;
		GL::GetProgramiv(program, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			GLint size = 0;
			GL::GetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
			std::string log(std::max(1, size), '\0');
			GL::GetProgramInfoLog(program, size, nullptr, log.data());
			GL::DeleteProgram(program);
			throw std::runtime_error("GL program link failed (" + key + "): " + log);
		}
		m_programs[key] = program;
		return program;
	}
	void OpenGLBackend::DeleteTargets()
	{
		if (m_framebuffer)
			GL::DeleteFramebuffers(1, &m_framebuffer);
		if (m_color)
			GL::DeleteRenderbuffers(1, &m_color);
		if (m_depth)
			GL::DeleteRenderbuffers(1, &m_depth);
		m_framebuffer = m_color = m_depth = 0;
	}
	void OpenGLBackend::Resize(int i_width, int i_height)
	{
		if (i_width < 1 || i_height < 1)
			return;
		DeleteTargets();
		m_width = i_width;
		m_height = i_height;
		GL::GenFramebuffers(1, &m_framebuffer);
		GL::BindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		GL::GenRenderbuffers(1, &m_color);
		GL::BindRenderbuffer(GL_RENDERBUFFER, m_color);
		GL::RenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, m_width, m_height);
		GL::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color);
		GL::GenRenderbuffers(1, &m_depth);
		GL::BindRenderbuffer(GL_RENDERBUFFER, m_depth);
		GL::RenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_DEPTH24_STENCIL8, m_width, m_height);
		GL::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth);
		if (GL::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			throw std::runtime_error("OpenGL framebuffer is incomplete");
		GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_width, m_height);
	}
	GLuint OpenGLBackend::GetTexture(const TexturePtr &i_texture)
	{
		auto texture = i_texture ? i_texture : TextureManager::GetInstance()->GetWhiteTexture();
		auto it = m_textures.find(texture->GetHandle());
		if (it != m_textures.end())
			return it->second;
		GLuint id = 0;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->m_width, texture->m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					 texture->m_pixels.data());
		GL::GenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_textures[texture->GetHandle()] = id;
		return id;
	}
	GLuint OpenGLBackend::GetCube(const TextureCubePtr &i_cube)
	{
		if (!i_cube)
			return 0;
		auto it = m_cubes.find(i_cube->GetHandle());
		if (it != m_cubes.end())
			return it->second;
		GLuint id = 0;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);
		for (int i = 0; i < 6; ++i)
		{
			auto &image = *i_cube->m_faces[i];
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, image.m_width, image.m_height, 0, GL_RGBA,
						 GL_UNSIGNED_BYTE, image.m_pixels.data());
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
			glTexParameteri(GL_TEXTURE_CUBE_MAP, wrap, GL_CLAMP_TO_EDGE);
		m_cubes[i_cube->GetHandle()] = id;
		return id;
	}
	OpenGLBackend::Buffers &OpenGLBackend::GetMesh(const MeshPtr &i_mesh)
	{
		auto it = m_meshes.find(i_mesh->GetHandle());
		if (it != m_meshes.end())
		{
			auto &buffers = it->second;
			if (buffers.revision != i_mesh->m_revision)
			{
				// 重用原VAO/VBO，驱动负责仍在飞行的旧存储，不增加缓存项。
				GL::BindVertexArray(buffers.m_vao);
				GL::BindBuffer(GL_ARRAY_BUFFER, buffers.m_vbo);
				GL::BufferData(GL_ARRAY_BUFFER, i_mesh->m_vertices.size() * sizeof(Vertex), i_mesh->m_vertices.data(),
							   GL_STATIC_DRAW);
				GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.m_ebo);
				GL::BufferData(GL_ELEMENT_ARRAY_BUFFER, i_mesh->m_indices.size() * sizeof(unsigned),
							   i_mesh->m_indices.data(), GL_STATIC_DRAW);
				buffers.revision = i_mesh->m_revision;
			}
			return buffers;
		}
		Buffers buffers;
		buffers.revision = i_mesh->m_revision;
		buffers.terrain = bool(i_mesh->m_terrain);
		buffers.owner = i_mesh;
		GL::GenVertexArrays(1, &buffers.m_vao);
		GL::BindVertexArray(buffers.m_vao);
		GL::GenBuffers(1, &buffers.m_vbo);
		GL::BindBuffer(GL_ARRAY_BUFFER, buffers.m_vbo);
		GL::BufferData(GL_ARRAY_BUFFER, i_mesh->m_vertices.size() * sizeof(Vertex), i_mesh->m_vertices.data(),
					   GL_STATIC_DRAW);
		GL::GenBuffers(1, &buffers.m_ebo);
		GL::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.m_ebo);
		GL::BufferData(GL_ELEMENT_ARRAY_BUFFER, i_mesh->m_indices.size() * sizeof(unsigned), i_mesh->m_indices.data(),
					   GL_STATIC_DRAW);
		GL::EnableVertexAttribArray(0);
		GL::EnableVertexAttribArray(1);
		GL::EnableVertexAttribArray(2);
		GL::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
								reinterpret_cast<void *>(offsetof(Vertex, _position)));
		GL::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
								reinterpret_cast<void *>(offsetof(Vertex, _normal)));
		GL::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
								reinterpret_cast<void *>(offsetof(Vertex, _texCoords)));
		return m_meshes.emplace(i_mesh->GetHandle(), buffers).first->second;
	}
	void OpenGLBackend::Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode)
	{
		// 阴影通道统一用默认着色器，贴图和透明裁剪仍取原材质。
		m_program = GetProgram(i_mode == 3 ? nullptr : i_item.m_material);
		GL::UseProgram(m_program);
		auto constants = MakeConstants(i_frame, i_item, i_mode);
		auto matrix = [&](const char *name, const glm::mat4 &value)
		{ GL::UniformMatrix4fv(GL::GetUniformLocation(m_program, name), 1, GL_FALSE, glm::value_ptr(value)); };
		auto vector = [&](const char *name, const glm::vec4 &value)
		{ GL::Uniform4fv(GL::GetUniformLocation(m_program, name), 1, glm::value_ptr(value)); };
		matrix("ModelToWorld_Matrix", constants.m_model);
		matrix("WorldToView_Matrix", constants.m_view);
		matrix("Projective_Matrix", constants.m_projection);
		matrix("Normal_Matrix", constants.m_normal);
		matrix("LightViewProjection_Matrix", constants.m_lightViewProjection);
		vector("shadowParams", constants.m_shadowParams);
		vector("viewPosition", constants.m_viewPosition);
		vector("materialColor", constants.m_color);
		vector("materialParams", constants.m_material);
		vector("surfaceParams", constants.m_surface);
		vector("sunDirection", constants.m_sunDirection);
		vector("sunColor", constants.m_sunColor);
		GL::Uniform4fv(GL::GetUniformLocation(m_program, "pointPosition"), 4,
					   glm::value_ptr(constants.m_pointPosition[0]));
		GL::Uniform4fv(GL::GetUniformLocation(m_program, "pointColor"), 4, glm::value_ptr(constants.m_pointColor[0]));
		GL::Uniform4fv(GL::GetUniformLocation(m_program, "pointAttenuation"), 4,
					   glm::value_ptr(constants.m_pointAttenuation[0]));
		GL::ActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GetTexture(i_item.m_material ? i_item.m_material->m_texture : nullptr));
		GL::Uniform1i(GL::GetUniformLocation(m_program, "basic_texture"), 0);
		GL::ActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, GetCube(i_frame.m_irradiance));
		GL::Uniform1i(GL::GetUniformLocation(m_program, "irradiance_texture"), 1);
		GL::ActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, GetCube(i_frame.m_sky));
		GL::Uniform1i(GL::GetUniformLocation(m_program, "cube_texture"), 2);
		GL::ActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, GetTexture(i_item.m_material ? i_item.m_material->m_opacityTexture : nullptr));
		GL::Uniform1i(GL::GetUniformLocation(m_program, "opacity_texture"), 3);
		GL::ActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, GetTexture(i_item.m_material ? i_item.m_material->m_normalTexture : nullptr));
		GL::Uniform1i(GL::GetUniformLocation(m_program, "normal_texture"), 4);
		GL::ActiveTexture(GL_TEXTURE5);
		// 写阴影图时不能同时把附件作为采样纹理绑定。
		glBindTexture(GL_TEXTURE_2D, i_mode == 3 ? GetTexture(nullptr) : m_shadowTexture);
		GL::Uniform1i(GL::GetUniformLocation(m_program, "shadow_texture"), 5);
		if (i_mode == 3)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}
		else if (i_mode == 1)
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			bool transparent = i_item.m_material && i_item.m_material->m_transparent;
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(i_mode == 2 || transparent ? GL_FALSE : GL_TRUE);
			if (transparent)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
				glDisable(GL_BLEND);
		}
		auto &buffers = GetMesh(i_item.m_mesh);
		GL::BindVertexArray(buffers.m_vao);
		const auto &part = i_item.m_mesh->m_subMeshes.at(i_item.m_submesh);
		glDrawElements(GL_TRIANGLES, part.m_indexCount, GL_UNSIGNED_INT,
					   reinterpret_cast<void *>(static_cast<uintptr_t>(part.m_firstIndex) * sizeof(unsigned)));
	}
	void OpenGLBackend::Render(const RenderFrame &i_frame)
	{
		// 场景/撤销历史都释放后回收地形缓存；GL驱动保留仍在执行的底层存储。
		for (auto it = m_meshes.begin(); it != m_meshes.end();)
		{
			if (it->second.terrain && it->second.owner.expired())
			{
				GL::DeleteVertexArrays(1, &it->second.m_vao);
				GL::DeleteBuffers(1, &it->second.m_vbo);
				GL::DeleteBuffers(1, &it->second.m_ebo);
				it = m_meshes.erase(it);
			}
			else
				++it;
		}
		if (m_profiling)
			GL::BeginQuery(GL_TIME_ELAPSED, m_timeQuery);
		GL::BindFramebuffer(GL_FRAMEBUFFER, m_shadowFramebuffer);
		glViewport(0, 0, ShadowSize, ShadowSize);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glClearColor(1, 1, 1, 1);
		glClearDepth(1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (i_frame.m_shadowParams.w > 0)
			for (const auto &item : i_frame.m_items)
				if (CastsShadow(i_frame, item))
					Draw(i_frame, item, 3);
		GL::BindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
		glViewport(0, 0, m_width, m_height);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glClearColor(.025f, .04f, .065f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glDisable(GL_CULL_FACE);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_MULTISAMPLE);
		GL::UseProgram(m_program);
		if (i_frame.m_drawSky && i_frame.m_sky)
		{
			DrawItem sky;
			sky.m_mesh = i_frame.m_skyMesh;
			sky.m_transform =
				glm::translate(glm::mat4(1), i_frame.m_camera.cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100));
			Draw(i_frame, sky, 2);
		}
		for (auto &item : i_frame.m_items)
			Draw(i_frame, item, item.m_sprite ? 1 : 0);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		GL::BindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
		GL::BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GL::BlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
		auto error = glGetError();
		if (m_profiling)
		{
			GL::EndQuery(GL_TIME_ELAPSED);
			GLuint64 time = 0;
			GL::GetQueryObjectui64v(m_timeQuery, GL_QUERY_RESULT, &time);
			m_gpuTime = time / 1000000.0;
		}
		if (error != GL_NO_ERROR)
			throw std::runtime_error("OpenGL draw error: " + std::to_string(error));
	}
	void OpenGLBackend::CaptureFrame(const std::filesystem::path &i_path)
	{
		std::vector<unsigned char> pixels(static_cast<size_t>(m_width) * m_height * 4);
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		Platform::WriteBMP(i_path, m_width, m_height, pixels.data(), true);
		Platform::Log("Captured frame: " + i_path.u8string());
	}
	void OpenGLBackend::Present(bool i_vsync)
	{
		if (m_swapInterval)
			m_swapInterval(i_vsync ? 1 : 0);
		if (!SwapBuffers(m_dc))
			throw std::runtime_error("OpenGL SwapBuffers failed");
	}
	void OpenGLBackend::ShutDown()
	{
		if (m_context)
		{
			wglMakeCurrent(m_dc, m_context);
			if (m_loaded)
			{
				if (m_timeQuery)
					GL::DeleteQueries(1, &m_timeQuery);
				for (auto &pair : m_meshes)
				{
					GL::DeleteVertexArrays(1, &pair.second.m_vao);
					GL::DeleteBuffers(1, &pair.second.m_vbo);
					GL::DeleteBuffers(1, &pair.second.m_ebo);
				}
				for (auto &pair : m_textures)
					glDeleteTextures(1, &pair.second);
				for (auto &pair : m_cubes)
					glDeleteTextures(1, &pair.second);
				DeleteTargets();
				if (m_shadowFramebuffer)
					GL::DeleteFramebuffers(1, &m_shadowFramebuffer);
				if (m_shadowTexture)
					glDeleteTextures(1, &m_shadowTexture);
				if (m_shadowDepth)
					GL::DeleteRenderbuffers(1, &m_shadowDepth);
				for (auto &pair : m_programs)
					GL::DeleteProgram(pair.second);
			}
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(m_context);
			m_context = nullptr;
		}
		if (m_dc)
		{
			ReleaseDC(m_window, m_dc);
			m_dc = nullptr;
		}
		m_program = 0;
		m_shadowFramebuffer = m_shadowTexture = m_shadowDepth = 0;
		m_timeQuery = 0;
		m_loaded = false;
		m_meshes.clear();
		m_textures.clear();
		m_cubes.clear();
		m_programs.clear();
	}
	std::unique_ptr<IRenderBackend> CreateOpenGLBackend()
	{
		return std::make_unique<OpenGLBackend>();
	}
} // namespace Rendering
