#include "Rendering/Backend/IRenderBackend.h"
#include "Rendering/TextureManager.h"
#include "Platform/Paths.h"
#include "Platform/Log.h"
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <gtc/matrix_transform.hpp>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
namespace Rendering
{
	using Microsoft::WRL::ComPtr;
	static void CheckDX(HRESULT i_result, const char *i_operation)
	{
		if (FAILED(i_result))
		{
			std::ostringstream message;
			message << i_operation << " failed, HRESULT=0x" << std::hex << static_cast<unsigned>(i_result);
			throw std::runtime_error(message.str());
		}
	}
	class DX11Backend final : public IRenderBackend
	{
	  public:
		~DX11Backend() override { ShutDown(); }
		void Initilize(void *i_pWindow, int i_width, int i_height, int i_samples, bool i_warp) override;
		void Resize(int i_width, int i_height) override;
		void Render(const RenderFrame &i_frame) override;
		void CaptureFrame(const std::filesystem::path &i_path) override;
		void Present(bool i_vsync) override;
		void ShutDown() override;
		void SetProfiling(bool i_enabled) override;
		double GetGPUTimeMilliseconds() const override { return m_gpuTime; }

	  private:
		struct Buffers
		{
			ComPtr<ID3D11Buffer> m_vertices, m_indices;
			uint64_t revision = 0;
			bool terrain = false;
			std::weak_ptr<Mesh> owner;
		};
		struct ShaderPair
		{
			ComPtr<ID3D11VertexShader> m_vertex;
			ComPtr<ID3D11PixelShader> m_pixel;
			ComPtr<ID3D11InputLayout> m_layout;
		};
		std::map<std::string, ShaderPair> m_shaders;
		ComPtr<ID3D11Device> m_device;
		ComPtr<ID3D11DeviceContext> m_context;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D11Texture2D> m_backBuffer, m_color, m_depth;
		static constexpr UINT ShadowSize = 2048;
		ComPtr<ID3D11Texture2D> m_shadowColor, m_shadowDepth;
		ComPtr<ID3D11RenderTargetView> m_shadowTarget;
		ComPtr<ID3D11DepthStencilView> m_shadowDepthTarget;
		ComPtr<ID3D11ShaderResourceView> m_shadowView;
		ComPtr<ID3D11RenderTargetView> m_renderTarget;
		ComPtr<ID3D11DepthStencilView> m_depthTarget;
		ComPtr<ID3D11VertexShader> m_vertexShader;
		ComPtr<ID3D11PixelShader> m_pixelShader;
		ComPtr<ID3D11InputLayout> m_layout;
		ComPtr<ID3D11Buffer> m_constants;
		ComPtr<ID3D11SamplerState> m_sampler, m_cubeSampler;
		ComPtr<ID3D11RasterizerState> m_rasterizer;
		ComPtr<ID3D11DepthStencilState> m_meshDepth, m_skyDepth, m_spriteDepth, m_shadowDepthState;
		ComPtr<ID3D11BlendState> m_spriteBlend;
		std::map<uint64_t, Buffers> m_meshes;
		std::map<uint64_t, ComPtr<ID3D11ShaderResourceView>> m_textures, m_cubes;
		int m_width = 0, m_height = 0, m_samples = 1;
		bool m_profiling = false;
		double m_gpuTime = -1;
		ComPtr<ID3D11Query> m_disjoint, m_timeStart, m_timeEnd;
		void WaitQuery(ID3D11Query *i_query, void *i_data, UINT i_size);
		Buffers &GetMesh(const MeshPtr &i_mesh);
		ID3D11ShaderResourceView *GetTexture(const TexturePtr &i_texture);
		ID3D11ShaderResourceView *GetCube(const TextureCubePtr &i_cube);
		ComPtr<ID3DBlob> Compile(const std::string &i_source, const char *i_entry, const char *i_profile);
		void Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode);
		ShaderPair &GetShaders(const MaterialPtr &i_material);
	};
	ComPtr<ID3DBlob> DX11Backend::Compile(const std::string &i_source, const char *i_entry, const char *i_profile)
	{
		ComPtr<ID3DBlob> shader, errors;
		HRESULT result = D3DCompile(
			i_source.data(), i_source.size(), "Assets/Shaders/DX11/Default.hlsl", nullptr, nullptr, i_entry, i_profile,
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR, 0, &shader, &errors);
		if (FAILED(result))
			throw std::runtime_error(std::string("HLSL compilation failed: ") +
									 (errors ? static_cast<const char *>(errors->GetBufferPointer()) : i_entry));
		return shader;
	}
	void DX11Backend::Initilize(void *i_pWindow, int i_width, int i_height, int i_samples, bool i_warp)
	{
		DXGI_SWAP_CHAIN_DESC swap = {};
		swap.BufferCount = 1;
		swap.BufferDesc.Width = i_width;
		swap.BufferDesc.Height = i_height;
		swap.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap.OutputWindow = static_cast<HWND>(i_pWindow);
		swap.SampleDesc.Count = 1;
		swap.Windowed = TRUE;
		swap.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		D3D_FEATURE_LEVEL requested = D3D_FEATURE_LEVEL_11_0, actual;
		CheckDX(D3D11CreateDeviceAndSwapChain(nullptr, i_warp ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
											  nullptr, 0, &requested, 1, D3D11_SDK_VERSION, &swap, &m_swapChain,
											  &m_device, &actual, &m_context),
				"Create DX11 device/swapchain (use --warp explicitly for software rendering)");
		ComPtr<IDXGIFactory> factory;
		ComPtr<IDXGIDevice> dxgiDevice;
		ComPtr<IDXGIAdapter> adapter;
		CheckDX(m_device.As(&dxgiDevice), "Query DXGI device");
		CheckDX(dxgiDevice->GetAdapter(&adapter), "Get DXGI adapter");
		CheckDX(adapter->GetParent(IID_PPV_ARGS(&factory)), "Get DXGI factory");
		CheckDX(factory->MakeWindowAssociation(swap.OutputWindow, DXGI_MWA_NO_ALT_ENTER),
				"Disable implicit fullscreen");
		for (int samples = 2; samples <= std::min(8, i_samples); samples *= 2)
		{
			UINT colorQuality = 0, depthQuality = 0;
			m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, samples, &colorQuality);
			m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, samples, &depthQuality);
			if (colorQuality && depthQuality)
				m_samples = samples;
		}
		auto source = Platform::ReadText(Platform::GetAssetPath("Shaders/DX11/Default.hlsl"));
		auto vertex = Compile(source, "VSMain", "vs_5_0"), pixel = Compile(source, "PSMain", "ps_5_0");
		CheckDX(
			m_device->CreateVertexShader(vertex->GetBufferPointer(), vertex->GetBufferSize(), nullptr, &m_vertexShader),
			"Create vertex shader");
		CheckDX(m_device->CreatePixelShader(pixel->GetBufferPointer(), pixel->GetBufferSize(), nullptr, &m_pixelShader),
				"Create pixel shader");
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, _position), D3D11_INPUT_PER_VERTEX_DATA,
			 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, _normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, _texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0}};
		CheckDX(m_device->CreateInputLayout(layout, 3, vertex->GetBufferPointer(), vertex->GetBufferSize(), &m_layout),
				"Create vertex layout");
		D3D11_BUFFER_DESC buffer = {};
		buffer.ByteWidth = sizeof(DrawConstants);
		buffer.Usage = D3D11_USAGE_DYNAMIC;
		buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		CheckDX(m_device->CreateBuffer(&buffer, nullptr, &m_constants), "Create constant buffer");
		D3D11_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler.MaxLOD = D3D11_FLOAT32_MAX;
		CheckDX(m_device->CreateSamplerState(&sampler, &m_sampler), "Create texture sampler");
		sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		CheckDX(m_device->CreateSamplerState(&sampler, &m_cubeSampler), "Create cubemap sampler");
		D3D11_RASTERIZER_DESC raster = {};
		raster.FillMode = D3D11_FILL_SOLID;
		raster.CullMode = D3D11_CULL_NONE;
		raster.FrontCounterClockwise = TRUE;
		raster.DepthClipEnable = TRUE;
		raster.MultisampleEnable = m_samples > 1;
		CheckDX(m_device->CreateRasterizerState(&raster, &m_rasterizer), "Create rasterizer state");
		D3D11_DEPTH_STENCIL_DESC depth = {};
		depth.DepthEnable = TRUE;
		depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depth.DepthFunc = D3D11_COMPARISON_LESS;
		CheckDX(m_device->CreateDepthStencilState(&depth, &m_shadowDepthState), "Create shadow depth state");
		depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		CheckDX(m_device->CreateDepthStencilState(&depth, &m_meshDepth), "Create mesh depth state");
		depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		CheckDX(m_device->CreateDepthStencilState(&depth, &m_skyDepth), "Create sky depth state");
		depth.DepthEnable = FALSE;
		CheckDX(m_device->CreateDepthStencilState(&depth, &m_spriteDepth), "Create sprite depth state");
		D3D11_BLEND_DESC blend = {};
		auto &target = blend.RenderTarget[0];
		target.BlendEnable = TRUE;
		target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		CheckDX(m_device->CreateBlendState(&blend, &m_spriteBlend), "Create sprite blend state");
		// 阴影固定分辨率、单采样，颜色存深度，独立深度附件负责遮挡。
		D3D11_TEXTURE2D_DESC shadow = {};
		shadow.Width = shadow.Height = ShadowSize;
		shadow.MipLevels = shadow.ArraySize = 1;
		shadow.Format = DXGI_FORMAT_R32_FLOAT;
		shadow.SampleDesc.Count = 1;
		shadow.Usage = D3D11_USAGE_DEFAULT;
		shadow.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		CheckDX(m_device->CreateTexture2D(&shadow, nullptr, &m_shadowColor), "Create shadow color");
		CheckDX(m_device->CreateRenderTargetView(m_shadowColor.Get(), nullptr, &m_shadowTarget), "Create shadow RTV");
		CheckDX(m_device->CreateShaderResourceView(m_shadowColor.Get(), nullptr, &m_shadowView), "Create shadow SRV");
		shadow.Format = DXGI_FORMAT_D32_FLOAT;
		shadow.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		CheckDX(m_device->CreateTexture2D(&shadow, nullptr, &m_shadowDepth), "Create shadow depth");
		CheckDX(m_device->CreateDepthStencilView(m_shadowDepth.Get(), nullptr, &m_shadowDepthTarget), "Create shadow DSV");
		float shadowClear[] = {1, 1, 1, 1};
		m_context->ClearRenderTargetView(m_shadowTarget.Get(), shadowClear);
		m_context->ClearDepthStencilView(m_shadowDepthTarget.Get(), D3D11_CLEAR_DEPTH, 1, 0);
		Resize(i_width, i_height);
		DXGI_ADAPTER_DESC description = {};
		adapter->GetDesc(&description);
		int length = WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, nullptr, 0, nullptr, nullptr);
		std::string gpu(std::max(1, length), '\0');
		WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, gpu.data(), length, nullptr, nullptr);
		if (!gpu.empty())
			gpu.pop_back();
		Platform::Log("Renderer: DX11, GPU=" + gpu + ", device=" + (i_warp ? "WARP" : "hardware") +
					  ", samples=" + std::to_string(m_samples));
	}
	void DX11Backend::Resize(int i_width, int i_height)
	{
		if (i_width < 1 || i_height < 1)
			return;
		m_context->OMSetRenderTargets(0, nullptr, nullptr);
		m_renderTarget.Reset();
		m_depthTarget.Reset();
		m_backBuffer.Reset();
		m_color.Reset();
		m_depth.Reset();
		CheckDX(m_swapChain->ResizeBuffers(0, i_width, i_height, DXGI_FORMAT_UNKNOWN, 0), "Resize DX11 swapchain");
		m_width = i_width;
		m_height = i_height;
		CheckDX(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&m_backBuffer)), "Get swapchain buffer");
		D3D11_TEXTURE2D_DESC color = {};
		color.Width = i_width;
		color.Height = i_height;
		color.MipLevels = 1;
		color.ArraySize = 1;
		color.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		color.SampleDesc.Count = m_samples;
		color.Usage = D3D11_USAGE_DEFAULT;
		color.BindFlags = D3D11_BIND_RENDER_TARGET;
		CheckDX(m_device->CreateTexture2D(&color, nullptr, &m_color), "Create scene color target");
		CheckDX(m_device->CreateRenderTargetView(m_color.Get(), nullptr, &m_renderTarget), "Create render target view");
		auto depth = color;
		depth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		CheckDX(m_device->CreateTexture2D(&depth, nullptr, &m_depth), "Create depth target");
		CheckDX(m_device->CreateDepthStencilView(m_depth.Get(), nullptr, &m_depthTarget), "Create depth target view");
	}
	DX11Backend::Buffers &DX11Backend::GetMesh(const MeshPtr &i_mesh)
	{
		auto it = m_meshes.find(i_mesh->GetHandle());
		if (it != m_meshes.end())
		{
			if (it->second.revision != i_mesh->m_revision)
			{
				m_context->UpdateSubresource(it->second.m_vertices.Get(), 0, nullptr, i_mesh->m_vertices.data(), 0, 0);
				m_context->UpdateSubresource(it->second.m_indices.Get(), 0, nullptr, i_mesh->m_indices.data(), 0, 0);
				it->second.revision = i_mesh->m_revision;
			}
			return it->second;
		}
		Buffers result;
		result.revision = i_mesh->m_revision;
		result.terrain = bool(i_mesh->m_terrain);
		result.owner = i_mesh;
		D3D11_BUFFER_DESC buffer = {};
		buffer.Usage = D3D11_USAGE_DEFAULT;
		buffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer.ByteWidth = static_cast<UINT>(i_mesh->m_vertices.size() * sizeof(Vertex));
		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = i_mesh->m_vertices.data();
		CheckDX(m_device->CreateBuffer(&buffer, &data, &result.m_vertices), "Upload vertices");
		buffer.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer.ByteWidth = static_cast<UINT>(i_mesh->m_indices.size() * sizeof(unsigned));
		data.pSysMem = i_mesh->m_indices.data();
		CheckDX(m_device->CreateBuffer(&buffer, &data, &result.m_indices), "Upload indices");
		return m_meshes.emplace(i_mesh->GetHandle(), std::move(result)).first->second;
	}
	ID3D11ShaderResourceView *DX11Backend::GetTexture(const TexturePtr &i_texture)
	{
		auto texture = i_texture ? i_texture : TextureManager::GetInstance()->GetWhiteTexture();
		auto it = m_textures.find(texture->GetHandle());
		if (it != m_textures.end())
			return it->second.Get();
		D3D11_TEXTURE2D_DESC info = {};
		info.Width = texture->m_width;
		info.Height = texture->m_height;
		info.MipLevels = 0;
		info.ArraySize = 1;
		info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		info.SampleDesc.Count = 1;
		info.Usage = D3D11_USAGE_DEFAULT;
		info.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		info.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = texture->m_pixels.data();
		data.SysMemPitch = texture->m_width * 4;
		ComPtr<ID3D11Texture2D> gpu;
		ComPtr<ID3D11ShaderResourceView> view;
		CheckDX(m_device->CreateTexture2D(&info, nullptr, &gpu), "Upload texture");
		m_context->UpdateSubresource(gpu.Get(), 0, nullptr, texture->m_pixels.data(), texture->m_width * 4, 0);
		CheckDX(m_device->CreateShaderResourceView(gpu.Get(), nullptr, &view), "Create texture view");
		m_context->GenerateMips(view.Get());
		return m_textures.emplace(texture->GetHandle(), std::move(view)).first->second.Get();
	}
	ID3D11ShaderResourceView *DX11Backend::GetCube(const TextureCubePtr &i_cube)
	{
		if (!i_cube)
			return nullptr;
		auto it = m_cubes.find(i_cube->GetHandle());
		if (it != m_cubes.end())
			return it->second.Get();
		D3D11_TEXTURE2D_DESC info = {};
		info.Width = i_cube->m_faces[0]->m_width;
		info.Height = i_cube->m_faces[0]->m_height;
		info.MipLevels = 1;
		info.ArraySize = 6;
		info.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		info.SampleDesc.Count = 1;
		info.Usage = D3D11_USAGE_IMMUTABLE;
		info.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		info.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		D3D11_SUBRESOURCE_DATA data[6] = {};
		for (int i = 0; i < 6; ++i)
		{
			data[i].pSysMem = i_cube->m_faces[i]->m_pixels.data();
			data[i].SysMemPitch = info.Width * 4;
		}
		ComPtr<ID3D11Texture2D> gpu;
		ComPtr<ID3D11ShaderResourceView> view;
		CheckDX(m_device->CreateTexture2D(&info, data, &gpu), "Upload cubemap");
		CheckDX(m_device->CreateShaderResourceView(gpu.Get(), nullptr, &view), "Create cubemap view");
		return m_cubes.emplace(i_cube->GetHandle(), std::move(view)).first->second.Get();
	}
	void DX11Backend::Draw(const RenderFrame &i_frame, const DrawItem &i_item, int i_mode)
	{
		auto &shaders = GetShaders(i_mode == 3 ? nullptr : i_item.m_material);
		m_context->VSSetShader(shaders.m_vertex.Get(), nullptr, 0);
		m_context->PSSetShader(shaders.m_pixel.Get(), nullptr, 0);
		m_context->IASetInputLayout(shaders.m_layout.Get());
		auto constants = MakeConstants(i_frame, i_item, i_mode);
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		CheckDX(m_context->Map(m_constants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map per-draw constants");
		memcpy(mapped.pData, &constants, sizeof(constants));
		m_context->Unmap(m_constants.Get(), 0);
		ID3D11Buffer *pConstants = m_constants.Get();
		m_context->VSSetConstantBuffers(0, 1, &pConstants);
		m_context->PSSetConstantBuffers(0, 1, &pConstants);
		ID3D11ShaderResourceView *views[] = {
			GetTexture(i_item.m_material ? i_item.m_material->m_texture : nullptr), GetCube(i_frame.m_irradiance),
			GetCube(i_frame.m_sky), GetTexture(i_item.m_material ? i_item.m_material->m_opacityTexture : nullptr),
			GetTexture(i_item.m_material ? i_item.m_material->m_normalTexture : nullptr),
			i_mode == 3 ? nullptr : m_shadowView.Get()};
		m_context->PSSetShaderResources(0, 6, views);
		ID3D11SamplerState *samplers[] = {m_sampler.Get(), m_cubeSampler.Get()};
		m_context->PSSetSamplers(0, 2, samplers);
		bool transparent = i_item.m_material && i_item.m_material->m_transparent;
		m_context->OMSetDepthStencilState(
			i_mode == 3 ? m_shadowDepthState.Get() :
			(i_mode == 1 ? m_spriteDepth.Get() : (i_mode == 2 || transparent ? m_skyDepth.Get() : m_meshDepth.Get())), 0);
		m_context->OMSetBlendState(i_mode == 1 || transparent ? m_spriteBlend.Get() : nullptr, nullptr, 0xffffffff);
		auto &mesh = GetMesh(i_item.m_mesh);
		ID3D11Buffer *pVertices = mesh.m_vertices.Get();
		UINT stride = sizeof(Vertex), offset = 0;
		m_context->IASetVertexBuffers(0, 1, &pVertices, &stride, &offset);
		m_context->IASetIndexBuffer(mesh.m_indices.Get(), DXGI_FORMAT_R32_UINT, 0);
		auto &part = i_item.m_mesh->m_subMeshes.at(i_item.m_submesh);
		m_context->DrawIndexed(part.m_indexCount, part.m_firstIndex, 0);
	}
	void DX11Backend::Render(const RenderFrame &i_frame)
	{
		// D3D11运行时延迟释放GPU仍在使用的buffer，移除CPU缓存即可。
		for (auto it = m_meshes.begin(); it != m_meshes.end();)
			if (it->second.terrain && it->second.owner.expired())
				it = m_meshes.erase(it);
			else
				++it;

		if (m_profiling)
		{
			m_context->Begin(m_disjoint.Get());
			m_context->End(m_timeStart.Get());
		}
		if (i_frame.m_shadowParams.w > 0)
		{
			// 先解除上一帧的读取绑定，不能一边采样一边写同一张纹理。
			ID3D11ShaderResourceView *empty = nullptr;
			m_context->PSSetShaderResources(5, 1, &empty);
			ID3D11RenderTargetView *shadow = m_shadowTarget.Get();
			m_context->OMSetRenderTargets(1, &shadow, m_shadowDepthTarget.Get());
			float clear[] = {1, 1, 1, 1};
			m_context->ClearRenderTargetView(shadow, clear);
			m_context->ClearDepthStencilView(m_shadowDepthTarget.Get(), D3D11_CLEAR_DEPTH, 1, 0);
			D3D11_VIEWPORT viewport = {0, 0, static_cast<float>(ShadowSize), static_cast<float>(ShadowSize), 0, 1};
			m_context->RSSetViewports(1, &viewport);
			m_context->RSSetState(m_rasterizer.Get());
			m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			for (const auto &item : i_frame.m_items)
				if (CastsShadow(i_frame, item))
					Draw(i_frame, item, 3);
			m_context->OMSetRenderTargets(0, nullptr, nullptr);
		}
		ID3D11RenderTargetView *pTarget = m_renderTarget.Get();
		m_context->OMSetRenderTargets(1, &pTarget, m_depthTarget.Get());
		float clear[] = {.025f, .04f, .065f, 1};
		m_context->ClearRenderTargetView(pTarget, clear);
		m_context->ClearDepthStencilView(m_depthTarget.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
		D3D11_VIEWPORT viewport = {0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0, 1};
		m_context->RSSetViewports(1, &viewport);
		m_context->RSSetState(m_rasterizer.Get());
		m_context->IASetInputLayout(m_layout.Get());
		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
		m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
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
		m_context->OMSetRenderTargets(0, nullptr, nullptr);
		if (m_samples > 1)
			m_context->ResolveSubresource(m_backBuffer.Get(), 0, m_color.Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		else
			m_context->CopyResource(m_backBuffer.Get(), m_color.Get());
		if (m_profiling)
		{
			m_context->End(m_timeEnd.Get());
			m_context->End(m_disjoint.Get());
			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = {};
			WaitQuery(m_disjoint.Get(), &disjoint, sizeof(disjoint));
			UINT64 start = 0, end = 0;
			WaitQuery(m_timeStart.Get(), &start, sizeof(start));
			WaitQuery(m_timeEnd.Get(), &end, sizeof(end));
			m_gpuTime = !disjoint.Disjoint && disjoint.Frequency ? (end - start) * 1000.0 / disjoint.Frequency : -1;
		}
	}
	void DX11Backend::SetProfiling(bool i_enabled)
	{
		m_profiling = i_enabled;
		if (i_enabled && !m_disjoint)
		{
			D3D11_QUERY_DESC desc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
			CheckDX(m_device->CreateQuery(&desc, &m_disjoint), "Create GPU disjoint query");
			desc.Query = D3D11_QUERY_TIMESTAMP;
			CheckDX(m_device->CreateQuery(&desc, &m_timeStart), "Create GPU start query");
			CheckDX(m_device->CreateQuery(&desc, &m_timeEnd), "Create GPU end query");
		}
	}
	void DX11Backend::WaitQuery(ID3D11Query *i_query, void *i_data, UINT i_size)
	{
		auto start = std::chrono::steady_clock::now();
		for (;;)
		{
			auto result = m_context->GetData(i_query, i_data, i_size, 0);
			CheckDX(result, "Read GPU timing query");
			if (result == S_OK)
				return;
			if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
				throw std::runtime_error("GPU timing query timed out");
			SwitchToThread();
		}
	}
	void DX11Backend::CaptureFrame(const std::filesystem::path &i_path)
	{
		D3D11_TEXTURE2D_DESC info = {};
		m_backBuffer->GetDesc(&info);
		info.BindFlags = 0;
		info.MiscFlags = 0;
		info.Usage = D3D11_USAGE_STAGING;
		info.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		ComPtr<ID3D11Texture2D> staging;
		CheckDX(m_device->CreateTexture2D(&info, nullptr, &staging), "Create frame readback");
		m_context->CopyResource(staging.Get(), m_backBuffer.Get());
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		CheckDX(m_context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Map frame readback");
		std::vector<unsigned char> pixels(static_cast<size_t>(m_width) * m_height * 4);
		for (int y = 0; y < m_height; ++y)
			memcpy(pixels.data() + static_cast<size_t>(y) * m_width * 4,
				   static_cast<const unsigned char *>(mapped.pData) + y * mapped.RowPitch, m_width * 4);
		m_context->Unmap(staging.Get(), 0);
		Platform::WriteBMP(i_path, m_width, m_height, pixels.data(), false);
		Platform::Log("Captured frame: " + i_path.u8string());
	}
	void DX11Backend::Present(bool i_vsync)
	{
		CheckDX(m_swapChain->Present(i_vsync ? 1 : 0, 0), "Present DX11 frame");
	}
	void DX11Backend::ShutDown()
	{
		if (m_context)
		{
			m_context->ClearState();
			m_context->Flush();
		}
		m_meshes.clear();
		m_textures.clear();
		m_cubes.clear();
		m_shaders.clear();
		m_disjoint.Reset();
		m_timeStart.Reset();
		m_timeEnd.Reset();
		m_shadowView.Reset();
		m_shadowTarget.Reset();
		m_shadowDepthTarget.Reset();
		m_shadowColor.Reset();
		m_shadowDepth.Reset();
		m_shadowDepthState.Reset();
		m_renderTarget.Reset();
		m_depthTarget.Reset();
		m_backBuffer.Reset();
		m_color.Reset();
		m_depth.Reset();
		m_vertexShader.Reset();
		m_pixelShader.Reset();
		m_layout.Reset();
		m_constants.Reset();
		m_sampler.Reset();
		m_cubeSampler.Reset();
		m_rasterizer.Reset();
		m_meshDepth.Reset();
		m_skyDepth.Reset();
		m_spriteDepth.Reset();
		m_spriteBlend.Reset();
		m_swapChain.Reset();
		m_context.Reset();
		m_device.Reset();
	}
	DX11Backend::ShaderPair &DX11Backend::GetShaders(const MaterialPtr &i_material)
	{
		std::string path = i_material ? i_material->m_dxShader : "Shaders/DX11/Default.hlsl";
		auto it = m_shaders.find(path);
		if (it != m_shaders.end())
			return it->second;
		ShaderPair pair;
		if (path == "Shaders/DX11/Default.hlsl")
		{
			pair.m_vertex = m_vertexShader;
			pair.m_pixel = m_pixelShader;
			pair.m_layout = m_layout;
		}
		else
		{
			auto source = Platform::ReadText(Platform::GetAssetPath(path));
			auto vs = Compile(source, "VSMain", "vs_5_0"), ps = Compile(source, "PSMain", "ps_5_0");
			CheckDX(m_device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &pair.m_vertex),
					"Create material vertex shader");
			CheckDX(m_device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pair.m_pixel),
					"Create material pixel shader");
			D3D11_INPUT_ELEMENT_DESC layout[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
												  offsetof(Vertex, _position), D3D11_INPUT_PER_VERTEX_DATA, 0},
												 {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
												  offsetof(Vertex, _normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
												 {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
												  offsetof(Vertex, _texCoords), D3D11_INPUT_PER_VERTEX_DATA, 0}};
			CheckDX(m_device->CreateInputLayout(layout, 3, vs->GetBufferPointer(), vs->GetBufferSize(), &pair.m_layout),
					"Create material input layout");
		}
		return m_shaders.emplace(path, std::move(pair)).first->second;
	}
	std::unique_ptr<IRenderBackend> CreateDX11Backend()
	{
		return std::make_unique<DX11Backend>();
	}
} // namespace Rendering
