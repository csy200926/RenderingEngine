-- 独立 Demo 和 Editor 共用的版本 1 场景；旋转为四元数 x,y,z,w。
return {
    version = 1,
    camera = { eye = {6,4,9}, direction = {-6,-3.5,-9}, fov = 55, near = 0.1, far = 200 },
    sky = {
        enabled = true,
        faces = {
            "Images/Rendering/skyBox/stormydays_ft.tga", "Images/Rendering/skyBox/stormydays_bk.tga",
            "Images/Rendering/skyBox/stormydays_up.tga", "Images/Rendering/skyBox/stormydays_dn.tga",
            "Images/Rendering/skyBox/stormydays_lf.tga", "Images/Rendering/skyBox/stormydays_rt.tga"
        }
    },
    actors = {
        {
            name = "旋转立方体 Cube", position = {0,0.5,0}, scale = {1.5,1,1},
            rotation = {0,0.25881904,0,0.96592583},
            components = {
                {type = "mesh", model = "builtin:cube", materials = {
                    {color = {0.2,0.55,0.9,1}, metallic = 0.2, roughness = 0.35, ao = 1,
                    alphaCutoff = 0, transparent = false, texture = "", normalTexture = "", opacityTexture = "",
                    vertexShader = "Shaders/OpenGL/DefaultVS.shader", fragmentShader = "Shaders/OpenGL/DefaultFS.shader",
                    dxShader = "Shaders/DX11/Default.hlsl"}
                }}
            },
            children = {
                {name = "普通子节点", position = {1.2,0.6,0}, scale = {0.3,0.3,0.3}, rotation = {0,0,0,1},
                components = {{type = "mesh", model = "builtin:cube"}}, children = {}}
            }
        },
        {
            name = "地面", position = {0,-0.25,0}, scale = {8,0.5,8}, rotation = {0,0,0,1},
            components = {{type = "mesh", model = "builtin:cube"}}, children = {}
        },
        {
            name = "太阳", position = {0,4,0}, scale = {1,1,1}, rotation = {0,0,0,1},
            components = {{type = "directional", color = {1,0.95,0.85}, direction = {-1,-1,-1},
                intensity = 2, attenuation = {0.2,0.03,0.01}}}, children = {}
        },
        {
            name = "点光", position = {2,2,2}, scale = {1,1,1}, rotation = {0,0,0,1},
            components = {{type = "point", color = {0.4,0.6,1}, direction = {-1,-1,-1},
                intensity = 4, attenuation = {1,0.09,0.032}}}, children = {}
        }
    }
}
