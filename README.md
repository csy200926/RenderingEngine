# RendererLite

从原 Renderer 项目分出的小渲染器，只保留 Vulkan / DX12、方块、地面和相机。先把一帧的正常流程读懂，再加材质和阴影。

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

## 从哪读

先看 [从 OpenGL 读懂 Vulkan / DX12](docs/READING.md)。文档假设你已会 OpenGL，用熟悉的 VAO/VBO、glUseProgram、uniform、FBO、glDrawElements 和 SwapBuffers 对照当前代码，再解释命令录制、显式资源状态与同步为何多出来。

发布包随附 `Source` 文本，文档可直接跳转代码；重新编译仍在完整工程根目录进行。

代码来自原工程的窗口/相机/几何体和两个后端的做法，重新整理为独立示例。没有复制 Actor、Director、场景树、Lua、Assimp、编辑器、Highlands 资源或旧引擎的资源系统。`Source/Scene.cpp` 是示例场景；`Shaders/Basic.hlsl` 是共享着色器。

## 用了什么

- Vulkan：Command Pool / Command Buffer、Graphics Pipeline、Descriptor Set、Push Constants；1.3 的 Dynamic Rendering、Synchronization2、`vkQueueSubmit2`，以及 1.2 的 Timeline Semaphore；用 swapchain maintenance1 的 Present Fence 安全回收呈现资源。两个在途帧各持有自己可覆写的资源。
- DX12：Command Allocator / Command List、Root Signature、PSO、Root Constants、Resource Barrier、Fence，同样两个在途帧。

Command Buffer 和 Pipeline 是基础机制，不是 Vulkan 新增特性。动态渲染减少 RenderPass/Framebuffer 对象；新版同步把用途与等待写得更直观，时间线同步用递增的完成值回收帧资源。

现在是简单方向光漫反射和小高光，不是 PBR；棋盘格由 Shader 算出，没有图片贴图、阴影、GI、光追、RenderGraph 或多线程录制。静态示例网格直接放在 CPU 可写的 GPU 可读内存，以减少上传代码；大型场景以后再改为 staging/default-device 内存。Vulkan 为让截图流程直观，支持时每帧复制到自己的 readback buffer，这有额外开销，不代表性能最优写法。

设计与进度见 `PLAN.md`，实际核验范围见 `docs/VERIFICATION.md`。源码中的中文注释解释各步为什么要做；首次阅读可以跳过创建信息的大量枚举，从 `Render` 看一帧的顺序。
