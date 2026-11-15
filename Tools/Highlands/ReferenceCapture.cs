using System;
using System.IO;
using System.Linq;
using System.Text;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;

// Offline capture only. Never saves the source scene or exports runtime assets.
public static class HighlandsReferenceCapture
{
    const string ScenePath = "Assets/HIGHLANDS - Stylized Environment/Demo/Demo.unity";
    static string Output => Path.GetFullPath(Path.Combine(Application.dataPath, "../../HighlandsReference"));

    public static void Configure()
    {
        PlayerSettings.colorSpace = ColorSpace.Linear;
        AssetDatabase.SaveAssets();
        Debug.Log("HIGHLANDS_REFERENCE_CONFIGURED Linear");
    }

    public static void Run()
    {
        Directory.CreateDirectory(Output);
        var report = new StringBuilder();
        try
        {
            if (QualitySettings.activeColorSpace != ColorSpace.Linear)
                throw new InvalidOperationException("Configure and restart Unity before capturing: Linear required.");
            if (GraphicsSettings.currentRenderPipeline != null)
                throw new InvalidOperationException("This capture requires Built-in Render Pipeline.");
            // QualitySettings writes through to the isolated project even without
            // SaveAssets; establish and restore source-quality shadow mode.
            QualitySettings.shadows = ShadowQuality.All;
            var scene = EditorSceneManager.OpenScene(ScenePath);
            foreach (var terrain in Terrain.activeTerrains)
            {
                var m = terrain.materialTemplate;
                report.AppendLine($"terrain={terrain.name}; sourceMaterial={m?.name}; sourceShader={m?.shader?.name}");
                if (m == null || m.shader == null || m.shader.name.Contains("InternalErrorShader") || !m.shader.isSupported)
                {
                    var shader=Shader.Find("Nature/Terrain/Standard");
                    if(shader==null || !shader.isSupported) throw new InvalidOperationException("Built-in Terrain Standard shader unavailable");
                    terrain.materialTemplate = new Material(shader);
                    report.AppendLine("CAPTURE-ONLY substitution: null/missing terrain material -> Built-in Nature/Terrain/Standard; original TerrainLayers retained; not saved.");
                }
            }
            report.AppendLine($"skyboxShader={RenderSettings.skybox?.shader?.name}; supported={RenderSettings.skybox?.shader?.isSupported}");
            var skyTexture=RenderSettings.skybox != null && RenderSettings.skybox.HasProperty("_Tex") ? RenderSettings.skybox.GetTexture("_Tex") : null;
            report.AppendLine($"skyboxTexture={skyTexture?.name}; type={skyTexture?.GetType().Name}; dimension={skyTexture?.dimension}; lightsUseLinearIntensity={GraphicsSettings.lightsUseLinearIntensity}; lightsUseColorTemperature={GraphicsSettings.lightsUseColorTemperature}");
            var objects = scene.GetRootGameObjects().SelectMany(x => x.GetComponentsInChildren<Transform>(true)).ToArray();
            var cameras = objects.Select(x => x.GetComponent<Camera>()).Where(x => x != null).ToArray();
            foreach (var c in cameras)
                report.AppendLine($"camera {c.name} enabled={c.enabled} active={c.gameObject.activeInHierarchy} pos={c.transform.position:R} forward={c.transform.forward:R} fov={c.fieldOfView:R}");
            var camera = cameras.Where(x => x.gameObject.activeInHierarchy).OrderBy(x => Vector3.Distance(x.transform.position, new Vector3(.6f,6.621f,-44.83f))).First();
            report.AppendLine($"Unity={Application.unityVersion}; pipeline=Built-in; colorSpace={QualitySettings.activeColorSpace}; graphics={SystemInfo.graphicsDeviceType}/{SystemInfo.graphicsDeviceName}");
            report.AppendLine($"chosenCamera={camera.name}; eye={camera.transform.position:R}; forward={camera.transform.forward:R}; fov={camera.fieldOfView:R}; clipping={camera.nearClipPlane:R},{camera.farClipPlane:R}; renderingPath={camera.actualRenderingPath}; HDR={camera.allowHDR}");
            report.AppendLine($"ambientMode={RenderSettings.ambientMode}; intensity={RenderSettings.ambientIntensity:R}; sky={RenderSettings.ambientSkyColor:R}; equator={RenderSettings.ambientEquatorColor:R}; ground={RenderSettings.ambientGroundColor:R}; skybox={RenderSettings.skybox?.name}");
            report.AppendLine($"fog={RenderSettings.fog}; mode={RenderSettings.fogMode}; color={RenderSettings.fogColor:R}; density={RenderSettings.fogDensity:R}; start={RenderSettings.fogStartDistance:R}; end={RenderSettings.fogEndDistance:R}; shadows={QualitySettings.shadows}; shadowDistance={QualitySettings.shadowDistance:R}");
            foreach (var light in objects.Select(x => x.GetComponent<Light>()).Where(x => x != null && x.isActiveAndEnabled))
                report.AppendLine($"light {light.name}; type={light.type}; color={light.color:R}; color.linear={light.color.linear:R}; intensity={light.intensity:R}; forward={light.transform.forward:R}; shadows={light.shadows}; sun={light==RenderSettings.sun}");
            int missingScripts = objects.Sum(x => GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(x.gameObject));
            var materials = objects.Select(x => x.GetComponent<Renderer>()).Where(x => x != null).SelectMany(x => x.sharedMaterials).Where(x => x != null).Distinct().ToArray();
            var shaders = materials.Select(x => x.shader).Where(x => x != null).Distinct().ToArray();
            report.AppendLine($"missingScripts={missingScripts}; rendererMaterials={materials.Length}; shaders={shaders.Length}; missingMaterialSlots=" + objects.Select(x=>x.GetComponent<Renderer>()).Where(x=>x!=null).Sum(x=>x.sharedMaterials.Count(m=>m==null)));
            // Source FX materials were migrated to an external URP shader. Omit them
            // rather than substitute materials or publish a magenta reference.
            foreach (var renderer in objects.Select(x => x.GetComponent<Renderer>()).Where(x => x != null))
            {
                var invalid = renderer.sharedMaterials.Where(m => m == null || m.shader == null || m.shader.name.Contains("InternalErrorShader") || !m.shader.isSupported).ToArray();
                if (invalid.Length == 0) continue;
                report.AppendLine($"OMITTED renderer={renderer.name}; materials=" + string.Join(",", invalid.Select(m => m == null ? "null" : m.name + "/" + m.shader?.name)));
                renderer.enabled = false;
            }
            report.AppendLine("Reference scope: Built-in original package main shaders; omitted missing-URP-shader FX, missing scripts/post effects are not reconstructed; no source scene/material saves.");
            shaders = objects.Select(x => x.GetComponent<Renderer>()).Where(x => x != null && x.enabled).SelectMany(x => x.sharedMaterials).Where(x => x != null && x.shader != null).Select(x => x.shader).Distinct().ToArray();
            ShaderUtil.allowAsyncCompilation = false;
            Capture(camera, "source.png");
            ExportAmbient();
            RenderSettings.fog = false;
            QualitySettings.shadows = ShadowQuality.Disable;
            foreach (var light in objects.Select(x => x.GetComponent<Light>()).Where(x => x != null)) light.shadows = LightShadows.None;
            Capture(camera, "source-basic.png");
            QualitySettings.shadows = ShadowQuality.All;
            int errors = 0;
            foreach (var shader in shaders)
            {
                report.AppendLine($"shader {shader.name}; supported={shader.isSupported}; asset={AssetDatabase.GetAssetPath(shader)}");
                foreach(var message in ShaderUtil.GetShaderMessages(shader))
                {
                    report.AppendLine($"  {message.severity}: {message.message} ({message.file}:{message.line})");
                    if(message.severity.ToString() == "Error") errors++;
                }
            }
            report.AppendLine("shaderErrors=" + errors);
            File.WriteAllText(Path.Combine(Output,"capture-report.txt"), report.ToString());
            if (errors > 0) throw new InvalidOperationException("Shader compilation errors: " + errors);
            Debug.Log("HIGHLANDS_REFERENCE_SUCCESS " + Output);
        }
        catch (Exception e)
        {
            report.AppendLine(e.ToString());
            File.WriteAllText(Path.Combine(Output,"capture-report.txt"),report.ToString());
            Debug.LogException(e);
            EditorApplication.Exit(1);
        }
    }

    [Serializable] class AmbientData
    {
        public string mode, colorSpace, coordinateConvention, coefficientOrder;
        public float intensity;
        public Color sky, equator, ground;
        public float[] sh27;
        public int faceSize;
        public string[] faceOrder;
        public float[] cubeLinearRGB;
    }

    static void ExportAmbient()
    {
        const int size=16;
        var probe=RenderSettings.ambientProbe;
        var data=new AmbientData { mode=RenderSettings.ambientMode.ToString(), colorSpace=QualitySettings.activeColorSpace.ToString(), intensity=RenderSettings.ambientIntensity, sky=RenderSettings.ambientSkyColor, equator=RenderSettings.ambientEquatorColor, ground=RenderSettings.ambientGroundColor, sh27=new float[27], faceSize=size, faceOrder=new[]{"+X","-X","+Y","-Y","+Z","-Z"}, cubeLinearRGB=new float[6*size*size*3], coefficientOrder="channel-major: sh27[channel*9+coefficient] = Unity SphericalHarmonicsL2[channel,coefficient]", coordinateConvention="cube directions follow Renderer TextureCube.GetDirection with top-first pixels in renderer coordinates; evaluated at Unity direction (x,y,-z), matching exporter handedness flip. Stored colors and SH are Unity raw Linear project values; cubeLinearRGB is SH.Evaluate output (unclamped). PNG encodes clamp01(linear)^(1/2.2), matching TextureCube::Sample gamma decode." };
        for(int c=0;c<3;c++) for(int i=0;i<9;i++) data.sh27[c*9+i]=probe[c,i];
        for(int face=0;face<6;face++)
        {
            var directions=new Vector3[size*size];
            var colors=new Color[size*size];
            for(int y=0;y<size;y++) for(int x=0;x<size;x++)
            {
                float u=2*(x+.5f)/size-1, v=2*(y+.5f)/size-1;
                var ds=new[]{new Vector3(1,-v,-u),new Vector3(-1,-v,u),new Vector3(u,1,v),new Vector3(u,-1,-v),new Vector3(u,-v,1),new Vector3(-u,-v,-1)};
                var d=ds[face].normalized;
                directions[y*size+x]=new Vector3(d.x,d.y,-d.z);
            }
            probe.Evaluate(directions,colors);
            var image=new Texture2D(size,size,TextureFormat.RGB24,false,false);
            for(int y=0;y<size;y++) for(int x=0;x<size;x++)
            {
                var c=colors[y*size+x];
                int offset=(face*size*size+y*size+x)*3;
                data.cubeLinearRGB[offset]=c.r; data.cubeLinearRGB[offset+1]=c.g; data.cubeLinearRGB[offset+2]=c.b;
                image.SetPixel(x,size-1-y,new Color(Mathf.Pow(Mathf.Clamp01(c.r),1/2.2f),Mathf.Pow(Mathf.Clamp01(c.g),1/2.2f),Mathf.Pow(Mathf.Clamp01(c.b),1/2.2f),1));
            }
            image.Apply();
            File.WriteAllBytes(Path.Combine(Output,"ambient-"+face+".png"),image.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(image);
        }
        File.WriteAllText(Path.Combine(Output,"ambient.json"),JsonUtility.ToJson(data,true));
    }

    static void Capture(Camera camera, string name)
    {
        var rt = new RenderTexture(1280,800,24,RenderTextureFormat.ARGB32,RenderTextureReadWrite.sRGB);
        var previousTarget = camera.targetTexture;
        var previousActive = RenderTexture.active;
        try
        {
            camera.targetTexture = rt;
            camera.aspect = 1280f/800f;
            for (int i=0;i<3;i++) camera.Render();
            RenderTexture.active = rt;
            var texture = new Texture2D(1280,800,TextureFormat.RGB24,false,false);
            texture.ReadPixels(new Rect(0,0,1280,800),0,0);
            texture.Apply();
            File.WriteAllBytes(Path.Combine(Output,name),texture.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(texture);
        }
        finally
        {
            camera.targetTexture = previousTarget;
            RenderTexture.active = previousActive;
            rt.Release();
            UnityEngine.Object.DestroyImmediate(rt);
        }
    }
}
