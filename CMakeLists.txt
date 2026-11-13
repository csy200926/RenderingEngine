cmake_minimum_required(VERSION 3.24)
project(RendererLite LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "RendererLite currently targets Windows + MSVC + Windows SDK")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
file(GLOB LITE_SOURCES CONFIGURE_DEPENDS Source/*.cpp)
add_executable(RendererLite ${LITE_SOURCES})
target_include_directories(RendererLite PRIVATE Source ThirdParty/glm ThirdParty/Vulkan/Include)
target_compile_definitions(RendererLite PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX GLM_FORCE_RADIANS)
target_compile_options(RendererLite PRIVATE /W3 /utf-8 /MP)
target_link_libraries(RendererLite PRIVATE user32 gdi32 d3d12 dxgi d3dcompiler)
set_target_properties(RendererLite PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RendererLite)

# 所有依赖来自此文件夹；不找 VULKAN_SDK，不联网下载，不要求配置 PATH。
set(LOCAL_DXC "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Vulkan/Tools/dxc.exe")
set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Basic.hlsl")
set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/Shaders")
add_custom_command(OUTPUT "${SHADER_OUTPUT}/Basic.vert.spv" "${SHADER_OUTPUT}/Basic.frag.spv"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT}"
    COMMAND "${LOCAL_DXC}" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -DVULKAN=1 -fvk-invert-y -T vs_6_0 -E VSMain -Fo "${SHADER_OUTPUT}/Basic.vert.spv" "${SHADER_SOURCE}"
    COMMAND "${LOCAL_DXC}" -spirv -fspv-target-env=vulkan1.3 -fvk-use-dx-layout -DVULKAN=1 -T ps_6_0 -E PSMain -Fo "${SHADER_OUTPUT}/Basic.frag.spv" "${SHADER_SOURCE}"
    DEPENDS "${SHADER_SOURCE}" "${LOCAL_DXC}" VERBATIM)
add_custom_target(LiteShaders DEPENDS "${SHADER_OUTPUT}/Basic.vert.spv" "${SHADER_OUTPUT}/Basic.frag.spv")
# Shader 单独变化时也复制；不能依赖 exe 重新链接才触发 POST_BUILD。
add_custom_target(CopyLiteShaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:RendererLite>/Shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SHADER_SOURCE}" "$<TARGET_FILE_DIR:RendererLite>/Shaders/Basic.hlsl"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_OUTPUT}" "$<TARGET_FILE_DIR:RendererLite>/Shaders"
    DEPENDS "${SHADER_SOURCE}" "${SHADER_OUTPUT}/Basic.vert.spv" "${SHADER_OUTPUT}/Basic.frag.spv")
add_dependencies(CopyLiteShaders LiteShaders)
add_dependencies(RendererLite CopyLiteShaders)
install(TARGETS RendererLite RUNTIME DESTINATION .)
install(FILES "${SHADER_SOURCE}" "${SHADER_OUTPUT}/Basic.vert.spv" "${SHADER_OUTPUT}/Basic.frag.spv" DESTINATION Shaders)
install(FILES README.md PLAN.md DESTINATION .)
install(DIRECTORY docs DESTINATION .)
# 学习文档可直接跳转源码；运行包随附这些文本，不参与运行时加载。
install(DIRECTORY Source DESTINATION .)
install(FILES Run-Vulkan.cmd Run-DX12.cmd DESTINATION .)
install(FILES ThirdParty/NOTICES.md ThirdParty/GLM-LICENSE.txt DESTINATION Licenses)
install(FILES ThirdParty/Vulkan/LICENSE-Apache-2.0.txt ThirdParty/Vulkan/LICENSE-Headers.md ThirdParty/Vulkan/LICENSE-MIT.txt DESTINATION Licenses/Vulkan)
install(FILES ThirdParty/Vulkan/Tools/LICENSE-LLVM.txt ThirdParty/Vulkan/Tools/LICENSE-MIT.txt ThirdParty/Vulkan/Tools/LICENSE-MS.txt DESTINATION Licenses/DXC)
