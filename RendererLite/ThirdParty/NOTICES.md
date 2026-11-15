# 本地依赖

只从原工程复制固定版本，不是运行时下载。

- GLM：原工程 ThirdParty/glm。其头文件带有完整许可，见 GLM-LICENSE.txt（包内随附）和 glm/detail/setup.hpp。保留头文件中的版权说明。
- Vulkan Headers：1.3.296，原工程 ThirdParty/Vulkan/Include。保留同目录 Vulkan 许可。
- DXC：原工程固定工具，1.8.2505.1；dxc.exe、dxcompiler.dll、dxil.dll 只用于构建期离线生成 SPIR-V。保留 Tools 目录的 LLVM/MIT/Microsoft 许可。
- DX12、DXGI 与 D3DCompiler：使用 Windows SDK/系统提供的组件，不复制系统 DLL。

本次精简版不使用 Lua、Assimp、stb、Unity 或原工程库。
