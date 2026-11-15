-- 基础几何体示例：Editor 和独立 Renderer 都可以打开。
-- 模型由自己的 PrimitiveMesh.cpp 生成；这里保存名称、摆放与简单材质。
return {
 version = 1,
 camera = {eye={0,7,10}, direction={0,-6.5,-11}, fov=55, near=0.1, far=200},
 sky = {enabled=true, faces={
  "Images/Rendering/skyBox/stormydays_ft.tga", "Images/Rendering/skyBox/stormydays_bk.tga",
  "Images/Rendering/skyBox/stormydays_up.tga", "Images/Rendering/skyBox/stormydays_dn.tga",
  "Images/Rendering/skyBox/stormydays_lf.tga", "Images/Rendering/skyBox/stormydays_rt.tga"}},
 actors = {
  {name="方块", position={-3,0.5,0}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:cube", materials={{color={0.3,0.55,0.85,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="球体", position={-1,0.5,0}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:sphere", materials={{color={0.85,0.35,0.3,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="圆柱", position={1,0.5,0}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:cylinder", materials={{color={0.3,0.75,0.55,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="圆锥", position={3,0.5,0}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:cone", materials={{color={0.9,0.65,0.25,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="胶囊", position={-2,0.5,-2}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:capsule", materials={{color={0.65,0.4,0.85,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="圆环", position={0,0.35,-2}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:torus", materials={{color={0.25,0.7,0.85,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="竖直平面", position={2,0.5,-2}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:quad", materials={{color={0.85,0.6,0.55,1}, metallic=0.25, roughness=0.35, ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="水平地面", position={0,-0.01,-1}, scale={10,1,7}, rotation={0,0,0,1},
   components={{type="mesh", model="builtin:plane", materials={{color={0.45,0.48,0.53,1},metallic=0,roughness=0.8,ao=1,alphaCutoff=0,transparent=false,texture="",normalTexture="",opacityTexture="",vertexShader="Shaders/OpenGL/DefaultVS.shader",fragmentShader="Shaders/OpenGL/DefaultFS.shader",dxShader="Shaders/DX11/Default.hlsl"}}}}, children={}},
  {name="太阳", position={0,4,0}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="directional",color={1,0.95,0.85},direction={-0.4,-1,-0.5},intensity=2,attenuation={1,0.09,0.032}}},children={}},
  {name="补光", position={-1,3,3}, scale={1,1,1}, rotation={0,0,0,1},
   components={{type="point",color={0.6,0.75,1},direction={-1,-1,-1},intensity=2,attenuation={1,0.09,0.032}}},children={}}
 }
}
