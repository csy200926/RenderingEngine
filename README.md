# RendererLite

轻量 Windows C++ 渲染器，提供 Vulkan 与 DirectX 12 两个后端，使用相同的场景数据和 HLSL Shader。当前包含基础几何体、自由相机、方向光、深度测试和截图。

## 开始

1. Windows 上准备 Visual Studio 2022 的“使用 C++ 的桌面开发”、Windows SDK，并准备 VS 内的 CMake 工具或独立安装的 CMake。
2. 双击 `Build.cmd`，或执行 `powershell -File Build.ps1 -Package`。
3. 双击 `Run-Vulkan.cmd` 或 `Run-DX12.cmd`。发布程序在 `Output`；调试工程通常在 `Build/RendererLite.sln`。整体移走时若发现旧缓存路径，脚本会另建 `Build-2` 等目录，不删除旧构建。

不需要配置 `VULKAN_SDK`、额外 PATH 或联网下载依赖。本文件夹带有固定版 GLM、Vulkan Headers 和构建期 DXC，可以整个移走；运行只需复制 `Output`。显卡驱动与系统图形运行库仍必须存在，Vulkan 后端明确要求 1.3、dynamicRendering / synchronization2 / timelineSemaphore，以及 VK_EXT_swapchain_maintenance1 和对应呈现维护扩展。缺少能力会明确报错，不会偷偷回退。

操作：WASD 移动，Q/E 升降，Shift 加速，按住鼠标右键转向，Esc 退出。默认场景静止，`--animate` 让方块旋转。

```powershell
./Output/RendererLite.exe --renderer vulkan
./Output/RendererLite.exe --renderer dx12 --animate
./Output/RendererLite.exe --renderer vulkan --hidden --frames 120 --capture frame.bmp --log run.log
```

## 项目结构

| 路径 | 内容 |
|---|---|
| `Source/Main.cpp` | 程序入口、相机更新和渲染循环 |
| `Source/Scene.*` | 几何体、场景数据和变换 |
| `Source/Vulkan*.cpp` | Vulkan 设备、资源、命令录制和呈现 |
| `Source/DX12*.cpp` | DX12 设备、资源、命令录制和呈现 |
| `Shaders/Basic.hlsl` | 两个后端共用的 Shader |
| `ThirdParty` | 固定版本的本地依赖和编译工具 |

发布包随附 `Source` 文本；重新编译在完整工程根目录进行。

工程由原 RenderingEngine 精简整理，不依赖旧工程的 Actor、Director、编辑器、Lua 或 Assimp。

## 用了什么

- Vulkan：Command Pool / Command Buffer、Graphics Pipeline、Descriptor Set、Push Constants；1.3 的 Dynamic Rendering、Synchronization2、`vkQueueSubmit2`，以及 1.2 的 Timeline Semaphore；用 swapchain maintenance1 的 Present Fence 安全回收呈现资源。两个在途帧各持有自己可覆写的资源。
- DX12：Command Allocator / Command List、Root Signature、PSO、Root Constants、Resource Barrier、Fence，同样两个在途帧。

Command Buffer 和 Pipeline 是基础机制，不是 Vulkan 新增特性。动态渲染减少 RenderPass/Framebuffer 对象；新版同步把用途与等待写得更直观，时间线同步用递增的完成值回收帧资源。

当前使用简单方向光漫反射和高光，棋盘格由 Shader 生成，尚未加入 PBR、图片材质、阴影、GI 或光追。静态网格使用 CPU 可写、GPU 可读内存。Vulkan 在支持截图时每帧复制到独立的 readback buffer，有额外传输开销。

资源与帧同步由各后端明确管理，两个在途帧分别持有可覆写的命令和常量资源。
