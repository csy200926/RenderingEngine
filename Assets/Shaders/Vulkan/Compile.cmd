@echo off
setlocal
rem Use the pinned local DXC without Vulkan SDK or PATH dependencies.
set "DXC=%~dp0..\..\..\ThirdParty\Vulkan\Tools\dxc.exe"
"%DXC%" -spirv -fspv-target-env=vulkan1.1 -fvk-use-dx-layout -fvk-b-shift 0 0 -fvk-t-shift 1 0 -fvk-s-shift 7 0 -fvk-invert-y -T vs_6_0 -E VSMain -Fo "%~dp0Default.vert.spv" "%~dp0Default.hlsl"
if errorlevel 1 exit /b 1
"%DXC%" -spirv -fspv-target-env=vulkan1.1 -fvk-use-dx-layout -fvk-b-shift 0 0 -fvk-t-shift 1 0 -fvk-s-shift 7 0 -T ps_6_0 -E PSMain -Fo "%~dp0Default.frag.spv" "%~dp0Default.hlsl"
if errorlevel 1 exit /b 1
exit /b 0
