# RenderingEngine

A clean, compact real-time renderer for Windows, written in C++ on top of modern graphics APIs.

One scene and material layer drives four interchangeable backends:

- Vulkan
- Direct3D 12
- Direct3D 11
- OpenGL 3.3

It includes PBR shading, shadow mapping, a small editor, and Lua scene files. `RendererLite/` is a minimal standalone Vulkan 1.3 renderer kept alongside as a readable reference.

## Build and run

Requires Visual Studio 2022 with the "Desktop development with C++" workload. All third-party dependencies are built from source in `ThirdParty/`.

```powershell
.\Build.cmd
.\Run-Vulkan.cmd
```

Other launchers: `Run-DX12.cmd`, `Run-DX11.cmd`, `Run-OpenGL.cmd`, `Run-Editor.cmd`, `Run-Sponza.cmd`, `Run-Highlands.cmd`.
