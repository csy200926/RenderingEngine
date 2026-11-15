// 一次性离线转换；只在隔离工程中运行，不参与引擎运行。
using System;

using System.IO;

using System.Linq;

using System.Text;

using System.Collections.Generic;

using System.Globalization;

using System.Security.Cryptography;

using System.Text.RegularExpressions;

using UnityEngine;

using UnityEditor;

using UnityEditor.SceneManagement;

using UnityEngine.SceneManagement;


public static class HighlandsExporter
{
    const string Package = "Assets/HIGHLANDS - Stylized Environment/";

    static string Root = Path.GetFullPath(Path.Combine(Application.dataPath, "../../..")) + "/";
    static string Out = Root + "Assets/Highlands/";

    static Dictionary<string, Model> models = new Dictionary<string, Model>();

    static Dictionary<string, Texture2D> images = new Dictionary<string, Texture2D>();

    class PixelData
    {
        public Color[] pixels;
        public int w, h;

    }
    static Dictionary<Texture2D, PixelData> pixelData = new Dictionary<Texture2D, PixelData>();
    static Dictionary<Texture2D, PixelData> linearPixelData = new Dictionary<Texture2D, PixelData>();
    static Dictionary<Texture, bool> colorTexturesSRGB = new Dictionary<Texture, bool>();
    static List<string> actors = new List<string>();

    static HashSet<Renderer> excluded = new HashSet<Renderer>();

    static Report report = new Report();
    static string SourceRoot = (Environment.GetEnvironmentVariable("HIGHLANDS_SOURCE_PROJECT") ?? "D:/Projects/PrototypeScene").TrimEnd('/', '\\') + "/";
    static Dictionary<Texture, bool> normalFlips = new Dictionary<Texture, bool>();
    static Dictionary<string, string> sourceTexturePaths = new Dictionary<string, string>();
    static Dictionary<string, string> shaderNames = new Dictionary<string, string>();
    [Serializable]
    public class MaterialRecord
    {
        public string file, sourceMaterial, sourceMaterialGuid, sourceShader, colorTexture, normalTexture;
        public List<string> sources = new List<string>(), notes = new List<string>();
        public float metallic, roughness, alphaCutoff;
        public bool transparent;
    }


    [Serializable]
    public class Report
    {
        public string sourceScene, sourceSceneSHA256, unityVersion, coordinates = "Z reflection; triangle winding reversed; original Unity UV; quaternion(-x,-y,z,w)";
        public int ordinaryRenderers, terrainTreeRenderers, sourceGameObjects, sourceRenderers, exportedRenderers, skippedInactive, skippedDisabled, skippedLOD, skippedParticles, missingMesh, terrainCount, terrainTrees, terrainDetails, uniqueModels, exportedMaterials, actors;
        public int normalMaterials, coverageMaterials;
        public List<MaterialRecord> materials = new List<MaterialRecord>();
        public List<string> missingResources = new List<string>();
        public List<string> limitations = new List<string>(), cameras = new List<string>(), landmarks = new List<string>(), terrains = new List<string>(), models = new List<string>();

    }
    class Model
    {
        public string path;
        public string[] materials;

    }
    class Props
    {
        public Dictionary<string, Texture2D> sampledImages = new Dictionary<string, Texture2D>();
        public string kind = "Standard";
        public bool Coverage { get { return kind == "Highlands_Surface" && F("_Coverageon") > .5f; } }
        public bool Transparent { get { return kind == "Standard" && (F("_Surface") == 1 || F("_Mode") >= 2); } }
        public string AlbedoKey { get { return kind.StartsWith("Highlands_") ? "_Albedo" : T("_BaseMap") ? "_BaseMap" : "_MainTex"; } }
        public Dictionary<string, float> f = new Dictionary<string, float>();
        public Dictionary<string, Color> c = new Dictionary<string, Color>();
        public Dictionary<string, Texture> t = new Dictionary<string, Texture>();
        public Dictionary<string, Vector4> uv = new Dictionary<string, Vector4>();

        public float F(string k, float d = 0)
        {
            return f.TryGetValue(k, out var v) ? v : d;

        }
        public Color C(string k, Color d)
        {
            return c.TryGetValue(k, out var v) ? v : d;

        }
        public Texture T(params string[] keys)
        {
            foreach (var k in keys)
                if (t.TryGetValue(k, out var v) && v)
                    return v;

            return null;

        }
        public Props(Material m)
        {
            if (!m)
                return;

            string materialPath = AssetDatabase.GetAssetPath(m);
            if (File.Exists(materialPath))
            {
                var match = Regex.Match(File.ReadAllText(materialPath), @"m_Shader:.*guid: ([a-f0-9]+)");
                if (match.Success && shaderNames.TryGetValue(match.Groups[1].Value, out var shaderName))
                {
                    kind = shaderName;
                }
            }
            var s = new SerializedObject(m);

            var saved = s.FindProperty("m_SavedProperties");

            Read(saved.FindPropertyRelative("m_Floats"), (k, v) => f[k] = v.floatValue);

            Read(saved.FindPropertyRelative("m_Colors"), (k, v) => c[k] = v.colorValue);

            Read(saved.FindPropertyRelative("m_TexEnvs"), (k, v) => { t[k] = v.FindPropertyRelative("m_Texture").objectReferenceValue as Texture; var a = v.FindPropertyRelative("m_Scale").vector2Value; var b = v.FindPropertyRelative("m_Offset").vector2Value; uv[k] = new Vector4(a.x, a.y, b.x, b.y); });

        }
        void Read(SerializedProperty a, Action<string, SerializedProperty> fn)
        {
            if (a == null)
                return;

            for (int i = 0; i < a.arraySize; i++)
            {
                var e = a.GetArrayElementAtIndex(i);

                fn(e.FindPropertyRelative("first").stringValue, e.FindPropertyRelative("second"));

            }
        }
    }
    static string F(float f)
    {
        return f.ToString("G9", CultureInfo.InvariantCulture);

    }
    static string Q(string s)
    {
        return "\"" + s.Replace("\\", "/").Replace("\"", "\\\"").Replace("\n", " ").Replace("\r", " ") + "\"";

    }
    static string V(Vector3 p)
    {
        return "{" + F(p.x) + "," + F(p.y) + "," + F(p.z) + "}";

    }
    static string V(Color p)
    {
        return "{" + F(p.r) + "," + F(p.g) + "," + F(p.b) + ",1}";

    }
    static Vector3 Flip(Vector3 p)
    {
        return new Vector3(p.x, p.y, -p.z);

    }
    static string Id(UnityEngine.Object o)
    {
        if (!o)
            return "null";

        if (AssetDatabase.TryGetGUIDAndLocalFileIdentifier(o, out string guid, out long local))
            return guid + "_" + local;

        return "builtin_" + o.name + "_" + o.GetInstanceID();

    }
    static string Hash(string s)
    {
        using (var sha = SHA256.Create())
            return BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(s))).Replace("-", "").ToLowerInvariant().Substring(0, 20);

    }
    static string SourceHash(string path)
    {
        using (var sha = SHA256.Create())
        using (var f = File.OpenRead(path))
            return BitConverter.ToString(sha.ComputeHash(f)).Replace("-", "").ToLowerInvariant();

    }
    static Texture2D Image(Texture tex)
    {
        if (!tex)
            return null;

        string p = AssetDatabase.GetAssetPath(tex);

        if (images.TryGetValue(p, out var old))
            return old;

        Texture2D result;

        if (p.EndsWith(".png", StringComparison.OrdinalIgnoreCase) || p.EndsWith(".jpg", StringComparison.OrdinalIgnoreCase))
        {
            result = new Texture2D(2, 2, TextureFormat.RGBA32, false);

            ImageConversion.LoadImage(result, File.ReadAllBytes(p));

        }
        else
        {
            var imp = AssetImporter.GetAtPath(p) as TextureImporter;

            if (imp != null)
            {
                imp.textureShape = TextureImporterShape.Texture2D;

                imp.isReadable = true;

                imp.textureCompression = TextureImporterCompression.Uncompressed;

                imp.maxTextureSize = 2048;

                imp.SaveAndReimport();

            }
            result = AssetDatabase.LoadAssetAtPath<Texture2D>(p);

        }
        images[p] = result;

        return result;

    }
    // 颜色采样先解码每个纹素，再双线性插值；法线、遮罩和透明度沿用原始数据采样。
    static Color Sample(Texture2D t, float u, float v, bool decodeSRGB = false)
    {
        if (ReferenceEquals(t, null))
            return Color.white;

        var cache = decodeSRGB ? linearPixelData : pixelData;
        if (!cache.TryGetValue(t, out var data))
        {
            data = new PixelData { pixels = t.GetPixels(), w = t.width, h = t.height };
            if (decodeSRGB)
            {
                for (int i = 0; i < data.pixels.Length; i++)
                    data.pixels[i] = data.pixels[i].linear;
            }
            cache[t] = data;

        }
        u = (u - Mathf.Floor(u)) * data.w - .5f;

        v = (v - Mathf.Floor(v)) * data.h - .5f;

        int ix = Mathf.FloorToInt(u), iy = Mathf.FloorToInt(v);

        float fx = u - ix, fy = v - iy;

        int ax = (ix % data.w + data.w) % data.w, bx = (ax + 1) % data.w, ay = (iy % data.h + data.h) % data.h, by = (ay + 1) % data.h;

        return Color.LerpUnclamped(Color.LerpUnclamped(data.pixels[ay * data.w + ax], data.pixels[ay * data.w + bx], fx), Color.LerpUnclamped(data.pixels[by * data.w + ax], data.pixels[by * data.w + bx], fx), fy);

    }
    // 源导入器决定颜色纹理是否为 sRGB；浮点 HDR 数据本身已经在线性空间。
    static Color SampleColor(Texture source, Texture2D image, float u, float v)
    {
        if (!source || !image)
            return Color.white;
        if (!colorTexturesSRGB.TryGetValue(source, out var decode))
        {
            var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(source)) as TextureImporter;
            decode = importer != null && importer.sRGBTexture && image.isDataSRGB;
            colorTexturesSRGB[source] = decode;
        }
        return Sample(image, u, v, decode);
    }
    // 普通 Color 属性在源 Linear 项目中由 Unity 转线性；HDR 属性保持其线性存储值。
    static Color LinearTint(Props p, string key, Color fallback)
    {
        var value = p.C(key, fallback);
        return p.kind == "Highlands_Background_Objects" && key == "_Color" ? value : value.linear;
    }
    // 引擎以 pow(rgb, 2.2) 解码，PNG 写入使用其逆变换，透明度始终保持原值。
    static Color EncodeColor(Color value)
    {
        return new Color(Mathf.Pow(Mathf.Max(0, value.r), 1 / 2.2f), Mathf.Pow(Mathf.Max(0, value.g), 1 / 2.2f), Mathf.Pow(Mathf.Max(0, value.b), 1 / 2.2f), value.a);
    }
    static float Cross(Vector2 a, Vector2 b)
    {
        return a.x * b.y - a.y * b.x;

    }
    static string Bake(Material mat, Mesh mesh, int sub, string key, out float cut)
    {
        var p = new Props(mat);

        var source = p.T(p.AlbedoKey);

        var image = Image(source);

        int w = image ? Math.Min(1024, image.width) : 64, h = image ? Math.Min(1024, image.height) : 64;

        var pixels = new Color[w * h];

        string baseKey = new[] { "_WaterColor", "_MainColor", "_BaseColor", "_Color" }.FirstOrDefault(p.c.ContainsKey) ?? "_Color";
        var baseColor = LinearTint(p, baseKey, Color.white);

        bool transparent = p.Transparent;

        var second = LinearTint(p, "_SecondColor", Color.white);

        bool secondOn = p.F("_Color2Enable") > .5f;

        float fade = p.F("_SecondColorFade", .5f), offset = p.F("_SecondColorOffset"), mode = p.F("_SecondColorOverlayType", 1);

        string texKey = p.AlbedoKey;

        var st = p.uv.TryGetValue(texKey, out var st0) ? st0 : new Vector4(1, 1, 0, 0);

        cut = Cutoff(p);

        report.exportedMaterials++;
        // 颜色烘焙不复用旧 PNG，避免旧版 gamma 混色结果继续命中缓存。
        var coverage = Coverage(p, mesh, sub, w, h);

        Func<float, float, float, Color> color = (u, v, y) =>
        {
            var c = SampleColor(source, image, u * st.x + st.z, v * st.y + st.w);
            float k = secondOn ? Mathf.Clamp01(((mode == 2 ? v : mode == 0 ? .5f : y) + offset) * fade * 3) : 0;
            var tint = Color.Lerp(baseColor, second, k);
            return new Color(c.r * tint.r, c.g * tint.g, c.b * tint.b, c.a * (transparent ? baseColor.a : 1));
        };

        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                var c = color((x + .5f) / w, (y + .5f) / h, mesh.bounds.center.y);
                if (coverage != null)
                {
                    var overlay = SampleColorProperty(p, "_CoverageAlbedo", (x + .5f) / w, (y + .5f) / h) * LinearTint(p, "_CoverageColor", Color.white);
                    c = new Color(Mathf.Lerp(c.r, overlay.r, coverage[y * w + x]), Mathf.Lerp(c.g, overlay.g, coverage[y * w + x]), Mathf.Lerp(c.b, overlay.b, coverage[y * w + x]), c.a);
                }
                pixels[y * w + x] = c;
            }

        // 顶点高度第二颜色通过UV光栅化固化到贴图，保留原贴图alpha。
        if (secondOn && mode == 1)
        {
            var uv = mesh.uv;

            var vertices = mesh.vertices;

            var indices = mesh.GetTriangles(sub);

            var seenUV = new HashSet<string>();

            long rasterBudget = 16000000;

            if (uv.Length == vertices.Length)
                for (int i = 0; i < indices.Length; i += 3)
                {
                    int a = indices[i], b = indices[i + 1], c = indices[i + 2];

                    var ua = uv[a];

                    var ub = uv[b];

                    var uc = uv[c];

                    string uvKey = ua.ToString("F5") + ub.ToString("F5") + uc.ToString("F5");

                    if (!seenUV.Add(uvKey) || rasterBudget <= 0)
                        continue;

                    float d = Cross(ub - ua, uc - ua);

                    if (Mathf.Abs(d) < 1e-8f)
                        continue;

                    int x0 = Mathf.Clamp(Mathf.FloorToInt(Mathf.Min(ua.x, Mathf.Min(ub.x, uc.x)) * w), 0, w - 1), x1 = Mathf.Clamp(Mathf.CeilToInt(Mathf.Max(ua.x, Mathf.Max(ub.x, uc.x)) * w), 0, w - 1), y0 = Mathf.Clamp(Mathf.FloorToInt(Mathf.Min(ua.y, Mathf.Min(ub.y, uc.y)) * h), 0, h - 1), y1 = Mathf.Clamp(Mathf.CeilToInt(Mathf.Max(ua.y, Mathf.Max(ub.y, uc.y)) * h), 0, h - 1);

                    rasterBudget -= (long)(x1 - x0 + 1) * (y1 - y0 + 1);

                    for (int y = y0; y <= y1; y++)
                        for (int x = x0; x <= x1; x++)
                        {
                            var v = new Vector2((x + .5f) / w, (y + .5f) / h);

                            float b1 = Cross(v - ua, uc - ua) / d, b2 = Cross(ub - ua, v - ua) / d, b0 = 1 - b1 - b2;

                            if (b0 >= -.001f && b1 >= -.001f && b2 >= -.001f)
                                pixels[y * w + x] = color(v.x, v.y, vertices[a].y * b0 + vertices[b].y * b1 + vertices[c].y * b2);

                        }
                }
        }
        var output = new Texture2D(w, h, TextureFormat.RGBA32, false);

        output.SetPixels(pixels.Select(EncodeColor).ToArray());

        output.Apply();

        string relative = "Highlands/Textures/" + key + ".png";

        File.WriteAllBytes(Root + "Assets/" + relative, output.EncodeToPNG());

        UnityEngine.Object.DestroyImmediate(output);

        return relative;

    }
    static float Cutoff(Props p)
    {
        if (p.Transparent || p.kind == "Highlands_Water")
        {
            return 0;
        }
        if (p.kind == "Highlands_Background_Objects")
        {
            return p.F("_CustomCutoff");
        }
        if (p.kind.StartsWith("Highlands_"))
        {
            return p.F("_AlphaCutoff");
        }
        return p.F("_AlphaClip") > .5f || p.F("_Mode") == 1 ? p.F("_Cutoff", .5f) : 0;
    }
    static Color SampleProperty(Props p, string name, float u, float v)
    {
        var st = p.uv.TryGetValue(name, out var value) ? value : new Vector4(1, 1, 0, 0);
        if (!p.sampledImages.TryGetValue(name, out var image))
        {
            image = Image(p.T(name));
            p.sampledImages[name] = image;
        }
        return Sample(image, u * st.x + st.z, v * st.y + st.w);
    }
    // 仅颜色贴图使用此路径，数据贴图继续走 SampleProperty。
    static Color SampleColorProperty(Props p, string name, float u, float v)
    {
        var st = p.uv.TryGetValue(name, out var value) ? value : new Vector4(1, 1, 0, 0);
        if (!p.sampledImages.TryGetValue(name, out var image))
        {
            image = Image(p.T(name));
            p.sampledImages[name] = image;
        }
        return SampleColor(p.T(name), image, u * st.x + st.z, v * st.y + st.w);
    }
    // 同一模型可被旋转复用，世界法线覆盖只能静态近似为模型局部法线，明确记录该限制。
    static float[] Coverage(Props p, Mesh mesh, int sub, int w, int h)
    {
        if (!p.Coverage)
        {
            return null;
        }
        var mask = new float[w * h];
        var uv = mesh.uv;
        var vertices = mesh.vertices;
        var normals = mesh.normals;
        var tris = mesh.GetTriangles(sub);
        bool position = p.F("_OverlayMethod") == 1;
        long budget = 16000000;
        for (int i = 0; i < tris.Length && budget > 0 && uv.Length == vertices.Length; i += 3)
        {
            int a = tris[i], b = tris[i + 1], c = tris[i + 2];
            Vector2 ua = uv[a], ub = uv[b], uc = uv[c];
            float d = Cross(ub - ua, uc - ua);
            if (Mathf.Abs(d) < 1e-8f)
            {
                continue;
            }
            int x0 = Mathf.Clamp(Mathf.FloorToInt(Mathf.Min(ua.x, Mathf.Min(ub.x, uc.x)) * w), 0, w - 1);
            int x1 = Mathf.Clamp(Mathf.CeilToInt(Mathf.Max(ua.x, Mathf.Max(ub.x, uc.x)) * w), 0, w - 1);
            int y0 = Mathf.Clamp(Mathf.FloorToInt(Mathf.Min(ua.y, Mathf.Min(ub.y, uc.y)) * h), 0, h - 1);
            int y1 = Mathf.Clamp(Mathf.CeilToInt(Mathf.Max(ua.y, Mathf.Max(ub.y, uc.y)) * h), 0, h - 1);
            budget -= (long)(x1 - x0 + 1) * (y1 - y0 + 1);
            for (int y = y0; y <= y1; y++)
            {
                for (int x = x0; x <= x1; x++)
                {
                    var point = new Vector2((x + .5f) / w, (y + .5f) / h);
                    float b1 = Cross(point - ua, uc - ua) / d, b2 = Cross(ub - ua, point - ua) / d, b0 = 1 - b1 - b2;
                    if (b0 < -.001f || b1 < -.001f || b2 < -.001f)
                    {
                        continue;
                    }
                    float height = position ? vertices[a].y * b0 + vertices[b].y * b1 + vertices[c].y * b2 : normals.Length == vertices.Length ? (normals[a] * b0 + normals[b] * b1 + normals[c] * b2).normalized.y : 0;
                    float contrast = p.F("_MaskContrast", 1);
                    float source = SampleProperty(p, "_CoverageMask", point.x, point.y).r;
                    source = Mathf.Clamp01(source * contrast + .5f * (1 - contrast));
                    mask[y * w + x] = Mathf.Clamp01((height + p.F("_CoverageLevel")) * source * p.F("_CoverageFade", 1) * 5);
                }
            }
        }
        return mask;
    }
    static Vector3 ReadNormal(Props p, string field, float u, float v, float scale)
    {
        var texture = p.T(field);
        if (!texture)
        {
            return Vector3.forward;
        }
        var color = SampleProperty(p, field, u, v);
        if (!normalFlips.TryGetValue(texture, out var flipGreen))
        {
            var importer = AssetImporter.GetAtPath(AssetDatabase.GetAssetPath(texture)) as TextureImporter;
            flipGreen = importer != null && importer.flipGreenChannel;
            normalFlips[texture] = flipGreen;
        }
        float nx = (color.r * 255 - 128) / 127;
        float ny = (color.g * 255 - 128) / 127 * (flipGreen ? -1 : 1);
        // 对应桌面UnpackScaleNormalRGorAG：先缩放XY，再由缩放值重建Z。
        // 源PNG是标准RGB法线，不读取Unity DXT5nm打包后的通道。
        nx *= scale;
        ny *= scale;
        float nz = Mathf.Sqrt(1 - Mathf.Clamp01(nx * nx + ny * ny));
        return new Vector3(nx, ny, nz).normalized;
    }
    static string ExportNormal(Props p, Mesh mesh, int sub, string key, MaterialRecord record)
    {
        string field = p.kind.StartsWith("Highlands_") ? "_Normal" : "_BumpMap";
        bool baseOn = p.T(field);
        bool overlayOn = p.Coverage && p.T("_CoverageNormal");
        if (!baseOn && !overlayOn)
        {
            return "";
        }
        foreach (string slot in new[] { field, overlayOn ? "_CoverageNormal" : "" })
        {
            var texture = p.T(slot);
            if (!texture)
            {
                continue;
            }
            string path = AssetDatabase.GetAssetPath(texture);
            var importer = AssetImporter.GetAtPath(path) as TextureImporter;
            if (!path.EndsWith(".png", StringComparison.OrdinalIgnoreCase) || (importer != null && importer.convertToNormalmap))
            {
                record.notes.Add("Unsupported normal source (height conversion/non-PNG): " + path);
                return "";
            }
        }
        var image = Image(p.T(baseOn ? field : "_CoverageNormal"));
        int w = Math.Min(1024, image.width), h = Math.Min(1024, image.height);
        var coverage = Coverage(p, mesh, sub, w, h);
        var colors = new Color[w * h];
        float scale = p.F(field == "_Normal" ? "_NormalScale" : "_BumpScale", 1);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                float u = (x + .5f) / w, v = (y + .5f) / h;
                var n = ReadNormal(p, field, u, v, scale);
                if (overlayOn)
                {
                    var overlay = ReadNormal(p, "_CoverageNormal", u, v, p.F("_CoverageNormalScale", 1));
                    if (p.F("_BlendNormals") > .5f)
                    {
                        overlay = new Vector3(n.x + overlay.x, n.y + overlay.y, n.z * overlay.z).normalized;
                    }
                    n = Vector3.Lerp(n, overlay, coverage[y * w + x]).normalized;
                }
                // Assimp翻V后TBN的B取反；仅翻绿色，不翻PNG行序。
                colors[y * w + x] = new Color(Mathf.Clamp01((n.x * 127 + 128) / 255), Mathf.Clamp01((-n.y * 127 + 128) / 255), n.z * .5f + .5f, 1);
            }
        }
        var output = new Texture2D(w, h, TextureFormat.RGB24, false, true);
        output.SetPixels(colors);
        output.Apply();
        string relative = "Highlands/Textures/" + key + "_normal.png";
        File.WriteAllBytes(Root + "Assets/" + relative, output.EncodeToPNG());
        UnityEngine.Object.DestroyImmediate(output);
        report.normalMaterials++;
        return relative;
    }
    static string ExportMaterial(Material mat, Props p, Mesh mesh, int sub, string texture, string key, float cutoff, StringBuilder mtl)
    {
        string name = Regex.Replace(mat ? mat.name : "Default", @"[^a-zA-Z0-9_()-]", "_");
        string relative = "Highlands/Materials/" + name + "_" + key + ".material";
        var record = new MaterialRecord { file = relative, sourceMaterial = AssetDatabase.GetAssetPath(mat), sourceMaterialGuid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(mat)), sourceShader = p.kind, colorTexture = texture, alphaCutoff = cutoff, transparent = p.Transparent };
        foreach (var pair in p.t.Where(x => x.Value))
        {
            string path = AssetDatabase.GetAssetPath(pair.Value);
            record.sources.Add(pair.Key + "=" + path + " guid=" + AssetDatabase.AssetPathToGUID(path));
        }
        record.normalTexture = ExportNormal(p, mesh, sub, key, record);
        record.notes.Add("Normal RGB PNG: source importer flipGreen applied, then engine V/TBN green reversal; XY scale normalized; no image row flip; max 1024 pixels per axis.");
        if (p.kind == "Highlands_Water")
        {
            record.notes.Add("Water color retained as static opaque approximation; animated world-space wave normal, refraction/depth transparency/foam are unsupported.");
        }
        if (File.Exists(record.sourceMaterial))
        {
            foreach (Match reference in Regex.Matches(File.ReadAllText(record.sourceMaterial), @"m_Texture:.*guid: ([a-f0-9]+)"))
            {
                string guid = reference.Groups[1].Value;
                if (string.IsNullOrEmpty(AssetDatabase.GUIDToAssetPath(guid)))
                {
                    if (sourceTexturePaths.TryGetValue(guid, out var sourcePath))
                    {
                        record.notes.Add("Source texture exists but was not imported by conversion: " + sourcePath + " guid=" + guid);
                        record.sources.Add("unimported=" + sourcePath + " guid=" + guid);
                    }
                    else
                    {
                        string missing = record.sourceMaterial + " texture guid=" + guid;
                        if (!report.missingResources.Contains(missing))
                        {
                            report.missingResources.Add(missing);
                        }
                    }
                }
            }
        }
        var coverage = Coverage(p, mesh, sub, 64, 64);
        double metallic = 0, smoothness = 0;
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                float u = (x + .5f) / 64, v = (y + .5f) / 64;
                Color albedo = SampleProperty(p, p.AlbedoKey, u, v);
                float metal = 0, smooth = 0;
                if (p.kind == "Highlands_Surface")
                {
                    var packed = SampleProperty(p, "_MetallicSmoothness", u, v);
                    metal = p.F("_Metallic") * packed.r;
                    int channel = (int)p.F("_SmoothnessSource");
                    smooth = p.F("_SurfaceSmoothness") * (channel == 1 ? albedo.a : channel == 2 ? packed.r : packed.a);
                }
                else if (p.kind == "Highlands_Foliage")
                {
                    var packed = SampleProperty(p, "_SmoothnessTexture", u, v);
                    int channel = (int)p.F("_SmoothnessSource");
                    smooth = p.F("_Smoothness") * (channel == 2 ? albedo.a : channel == 1 ? packed.a : packed.r);
                }
                else if (p.kind == "Highlands_Water")
                {
                    smooth = p.F("_Smoothness", .96f);
                }
                else if (p.kind == "Standard")
                {
                    var packed = SampleProperty(p, "_MetallicGlossMap", u, v);
                    metal = p.T("_MetallicGlossMap") ? packed.r : p.F("_Metallic");
                    bool urp = p.f.ContainsKey("_Surface");
                    float factor = p.F(urp ? "_Smoothness" : "_Glossiness", .5f);
                    smooth = p.F("_SmoothnessTextureChannel") == 1 ? albedo.a * factor : p.T("_MetallicGlossMap") ? packed.a * (urp ? factor : p.F("_GlossMapScale", 1)) : factor;
                }
                if (coverage != null)
                {
                    float k = coverage[y * 64 + x];
                    var packed = SampleProperty(p, "_CoverageSmoothnessTexture", u, v);
                    int channel = (int)p.F("_CoverageSmoothnessSource");
                    float overlay = p.F("_CoverageSmoothness") * (channel == 2 ? albedo.a : channel == 1 ? packed.a : packed.r);
                    metal *= 1 - k;
                    smooth = Mathf.Lerp(smooth, overlay, k);
                }
                metallic += Mathf.Clamp01(metal);
                smoothness += Mathf.Clamp01(smooth);
            }
        }
        record.metallic = (float)(metallic / 4096);
        record.roughness = Mathf.Clamp(1 - (float)(smoothness / 4096), .045f, 1);
        record.notes.Add("Texture metallic/smoothness reduced to a 64x64 UV-domain scalar mean; no additional map slots.");
        if (p.Coverage)
        {
            report.coverageMaterials++;
            record.notes.Add("Coverage RGB and normal statically baked using local geometric normal/position and source mask/ST. World orientation, normal-perturbed mask and repeated/overlapping UV cannot be represented exactly by shared model textures.");
        }
        if (p.T("_DetailNormalMap"))
        {
            record.notes.Add("Detail normal not exported.");
        }
        string text = "Shaders/OpenGL/DefaultVS.shader\nShaders/OpenGL/DefaultFS.shader\nDX11=Shaders/DX11/Default.hlsl\nTexture=" + texture + "\nNormalTexture=" + record.normalTexture + "\nOpacityTexture=\nColor=1,1,1,1\nMetallic=" + F(record.metallic) + "\nRoughness=" + F(record.roughness) + "\nAO=1\nAlphaCutoff=" + F(cutoff) + "\nTransparent=" + (p.Transparent ? "true" : "false") + "\n";
        File.WriteAllText(Root + "Assets/" + relative, text, new UTF8Encoding(false));
        mtl.AppendLine("# RendererMaterial material_" + sub + " ../Materials/" + Path.GetFileName(relative));
        mtl.AppendLine("newmtl material_" + sub + "\nKd 1 1 1\nPm " + F(record.metallic) + "\nPr " + F(record.roughness) + "\nd " + F(p.Transparent ? p.C("_BaseColor", p.C("_Color", Color.white)).a : 1) + "\nillum 2\nmap_Kd ../Textures/" + Path.GetFileName(texture));
        if (record.normalTexture.Length > 0)
        {
            mtl.AppendLine("map_Bump ../Textures/" + Path.GetFileName(record.normalTexture));
        }
        if (cutoff > 0 || p.Transparent)
        {
            record.notes.Add("Alpha is retained in baked RGBA color; material file defines cutoff/transparent mode.");
        }
        report.materials.Add(record);
        return "{file=" + Q(relative) + "}";
    }
    static Model ExportModel(Mesh mesh, Material[] materials)
    {
        string key = Id(mesh) + "|" + string.Join("|", materials.Select(Id));

        if (models.TryGetValue(key, out var model))
            return model;

        string id = Hash(key);

        model = new Model { path = "Highlands/Models/" + id + ".obj", materials = new string[mesh.subMeshCount] };

        var obj = new StringBuilder("# Highlands offline conversion\nmtllib " + id + ".mtl\n");

        var mtl = new StringBuilder();

        var vs = mesh.vertices;

        var ns = mesh.normals;

        var uvs = mesh.uv;

        foreach (var v in vs)
            obj.AppendLine("v " + F(v.x) + " " + F(v.y) + " " + F(-v.z));

        for (int i = 0; i < vs.Length; i++)
        {
            var uv = uvs.Length == vs.Length ? uvs[i] : Vector2.zero;

            obj.AppendLine("vt " + F(uv.x) + " " + F(uv.y));

        }
        for (int i = 0; i < vs.Length; i++)
        {
            var n = ns.Length == vs.Length ? ns[i] : Vector3.up;

            obj.AppendLine("vn " + F(n.x) + " " + F(n.y) + " " + F(-n.z));

        }
        for (int s = 0; s < mesh.subMeshCount; s++)
        {
            var mat = materials.Length > 0 ? materials[Math.Min(s, materials.Length - 1)] : null;

            string texture = Bake(mat, mesh, s, id + "_" + s, out float cutoff);

            var props = new Props(mat);

            model.materials[s] = ExportMaterial(mat, props, mesh, s, texture, id + "_" + s, cutoff, mtl);

            obj.AppendLine("g submesh_" + s + "\nusemtl material_" + s);



            var tris = mesh.GetTriangles(s);

            for (int i = 0; i < tris.Length; i += 3)
            {
                int a = tris[i] + 1, b = tris[i + 2] + 1, c = tris[i + 1] + 1;

                obj.AppendLine("f " + a + "/" + a + "/" + a + " " + b + "/" + b + "/" + b + " " + c + "/" + c + "/" + c);

            }
        }
        File.WriteAllText(Root + "Assets/" + model.path, obj.ToString(), new UTF8Encoding(false));

        File.WriteAllText(Out + "Models/" + id + ".mtl", mtl.ToString(), new UTF8Encoding(false));

        models[key] = model;

        Debug.Log("HIGHLANDS_MODEL " + models.Count + " " + mesh.name);

        report.models.Add(model.path + " <- " + AssetDatabase.GetAssetPath(mesh) + ":" + mesh.name + " vertices=" + vs.Length + " submeshes=" + mesh.subMeshCount);

        return model;

    }
    static string Node(string name, Vector3 pos, Quaternion rot, Vector3 scale, string component)
    {
        return "{name=" + Q(name) + ",position=" + V(Flip(pos)) + ",rotation={" + F(-rot.x) + "," + F(-rot.y) + "," + F(rot.z) + "," + F(rot.w) + "},scale=" + V(scale) + ",components={" + component + "},children={}},\n";

    }
    static HashSet<Renderer> ExcludeLODs(GameObject root)
    {
        var no = new HashSet<Renderer>();

        foreach (var group in root.GetComponentsInChildren<LODGroup>(true))
        {
            var lod = group.GetLODs();

            var keep = new HashSet<Renderer>(lod.Length > 0 ? lod[0].renderers : new Renderer[0]);

            foreach (var l in lod.Skip(1))
                foreach (var r in l.renderers)
                    if (r && !keep.Contains(r))
                        no.Add(r);

        }
        return no;

    }
    static void Renderer(Renderer renderer, Matrix4x4 matrix, string prefix, HashSet<Renderer> no)
    {
        if (no.Contains(renderer))
        {
            report.skippedLOD++;

            return;

        }
        if (renderer is ParticleSystemRenderer)
        {
            report.skippedParticles++;

            return;

        }
        if (!renderer.enabled)
        {
            report.skippedDisabled++;

            return;

        }
        var filter = renderer.GetComponent<MeshFilter>();

        Mesh mesh = filter ? filter.sharedMesh : (renderer as SkinnedMeshRenderer)?.sharedMesh;

        if (!mesh)
        {
            report.missingMesh++;

            return;

        }
        var scale = matrix.lossyScale;

        if (Mathf.Abs(scale.x * scale.y * scale.z) < 1e-10f)
        {
            report.skippedDisabled++;

            return;

        }
        var model = ExportModel(mesh, renderer.sharedMaterials);

        actors.Add(Node(prefix + renderer.name, matrix.GetColumn(3), matrix.rotation, scale, "{type=\"mesh\",model=" + Q(model.path) + ",materials={" + string.Join(",", model.materials) + "}}"));

        report.exportedRenderers++;

    }
    // 地形仅重烘颜色，不改高度、模型或树实例。
    static string BakeTerrainColor(TerrainData data, string id)
    {
        int size = 2048;

        var alpha = data.GetAlphamaps(0, 0, data.alphamapWidth, data.alphamapHeight);

        var layers = data.terrainLayers;

        var textures = layers.Select(x => Image(x.diffuseTexture)).ToArray();

        var colors = new Color[size * size];

        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                float u = (float)x / (size - 1), v = 1 - (float)y / (size - 1);

                int ax = Math.Min(data.alphamapWidth - 1, (int)(u * data.alphamapWidth)), az = Math.Min(data.alphamapHeight - 1, (int)(v * data.alphamapHeight));

                Color c = new Color(0, 0, 0, 1);

                for (int l = 0; l < layers.Length; l++)
                {
                    var layer = layers[l];

                    var t = SampleColor(layer.diffuseTexture, textures[l], (u * data.size.x + layer.tileOffset.x) / layer.tileSize.x, (v * data.size.z + layer.tileOffset.y) / layer.tileSize.y);

                    float weight = alpha[az, ax, l];

                    c.r += t.r * weight;

                    c.g += t.g * weight;

                    c.b += t.b * weight;

                }
                colors[(size - 1 - y) * size + x] = c;

            }
        var tex = new Texture2D(size, size, TextureFormat.RGB24, false);

        tex.SetPixels(colors.Select(EncodeColor).ToArray());

        tex.Apply();

        string albedo = "Highlands/Terrain/" + id + ".png";

        File.WriteAllBytes(Root + "Assets/" + albedo, tex.EncodeToPNG());

        UnityEngine.Object.DestroyImmediate(tex);

        return albedo;
    }
    static void Terrain(Terrain terrain)
    {
        var data = terrain.terrainData;

        int res = data.heightmapResolution;

        float[,] heights = data.GetHeights(0, 0, res, res);

        string id = "terrain_" + report.terrainCount;

        string albedo = BakeTerrainColor(data, id);

        string path = "Highlands/Terrain/" + id + ".terrain";

        using (var writer = new BinaryWriter(File.Create(Root + "Assets/" + path)))
        {
            writer.Write(new byte[] { 77, 89, 84, 82, 78, 48, 49, 0 });

            writer.Write((uint)res);

            writer.Write((uint)res);

            writer.Write(data.size.x);

            writer.Write(data.size.z);

            byte[] text = Encoding.UTF8.GetBytes(albedo);

            writer.Write((uint)text.Length);

            writer.Write(text);

            for (int z = 0; z < res; z++)
                for (int x = 0; x < res; x++)
                    writer.Write(heights[res - 1 - z, x] * data.size.y);

        }
        var pos = terrain.transform.position;

        actors.Add(Node(terrain.name, pos + new Vector3(0, 0, data.size.z), Quaternion.identity, Vector3.one, "{type=\"mesh\",model=" + Q(path) + "}"));

        report.terrains.Add(path + " resolution=" + res + " size=" + V(data.size) + " sourcePosition=" + V(pos) + " layers=" + data.terrainLayers.Length + " trees=" + data.treeInstanceCount);

        report.terrainCount++;

        report.terrainDetails += data.detailPrototypes.Length;

        foreach (var tree in data.treeInstances)
        {
            if (tree.prototypeIndex < 0 || tree.prototypeIndex >= data.treePrototypes.Length)
                continue;

            var prefab = data.treePrototypes[tree.prototypeIndex].prefab;

            if (!prefab)
                continue;

            var matrix = Matrix4x4.TRS(pos + Vector3.Scale(tree.position, data.size), Quaternion.Euler(0, tree.rotation * Mathf.Rad2Deg, 0), new Vector3(tree.widthScale, tree.heightScale, tree.widthScale));

            var no = ExcludeLODs(prefab);

            foreach (var renderer in prefab.GetComponentsInChildren<Renderer>(true))
            {
                var relative = prefab.transform.worldToLocalMatrix * renderer.localToWorldMatrix;

                Renderer(renderer, matrix * relative, "TerrainTree/", no);

            }
            report.terrainTrees++;

        }
    }
    // 直接导出源环境探针的辐照度，保持参考捕获的坐标、像素方向和编码，不再卷积天空。
    static string AmbientFaces()
    {
        const int size = 16;
        var probe = RenderSettings.ambientProbe;
        var paths = new string[6];
        for (int face = 0; face < 6; face++)
        {
            var directions = new Vector3[size * size];
            var colors = new Color[size * size];
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    float u = 2 * (x + .5f) / size - 1, v = 2 * (y + .5f) / size - 1;
                    Vector3 direction = face == 0 ? new Vector3(1, -v, -u) : face == 1 ? new Vector3(-1, -v, u) : face == 2 ? new Vector3(u, 1, v) : face == 3 ? new Vector3(u, -1, -v) : face == 4 ? new Vector3(u, -v, 1) : new Vector3(-u, -v, -1);
                    directions[y * size + x] = Flip(direction.normalized);
                }
            probe.Evaluate(directions, colors);
            var image = new Texture2D(size, size, TextureFormat.RGB24, false, false);
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++)
                {
                    var c = colors[y * size + x];
                    image.SetPixel(x, size - 1 - y, EncodeColor(new Color(Mathf.Clamp01(c.r), Mathf.Clamp01(c.g), Mathf.Clamp01(c.b), 1)));
                }
            image.Apply();
            paths[face] = "Highlands/Textures/ambient-" + face + ".png";
            File.WriteAllBytes(Root + "Assets/" + paths[face], image.EncodeToPNG());
            UnityEngine.Object.DestroyImmediate(image);
        }
        return "ambientFaces={" + string.Join(",", new[] { paths[5], paths[4], paths[2], paths[3], paths[1], paths[0] }.Select(Q)) + "}";
    }
    static string Sky()
    {
        var mat = RenderSettings.skybox;

        var p = new Props(mat);

        var source = Image(p.T("_Tex", "_MainTex"));

        string[] paths = new string[6];

        float rotation = p.F("_Rotation") * Mathf.Deg2Rad, exposure = p.F("_Exposure", 1);

        var tint = p.C("_Tint", Color.gray);

        for (int side = 0; side < 6; side++)
        {
            int n = 512;

            var colors = new Color[n * n];

            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float u = 2 * (x + .5f) / n - 1, v = 2 * (y + .5f) / n - 1;

                    Vector3 dir = side == 0 ? new Vector3(1, -v, -u) : side == 1 ? new Vector3(-1, -v, u) : side == 2 ? new Vector3(u, 1, v) : side == 3 ? new Vector3(u, -1, -v) : side == 4 ? new Vector3(u, -v, 1) : new Vector3(-u, -v, -1);

                    dir = Flip(dir.normalized);

                    dir = Quaternion.Euler(0, -rotation * Mathf.Rad2Deg, 0) * dir;

                    float pu = .5f - Mathf.Atan2(dir.z, dir.x) / (2 * Mathf.PI), pv = .5f + Mathf.Asin(dir.y) / Mathf.PI;

                    var c = Sample(source, pu, pv);

                    colors[(n - 1 - y) * n + x] = new Color(Mathf.Pow(Mathf.Max(0, c.r * tint.r * 2 * exposure), 1 / 2.2f), Mathf.Pow(Mathf.Max(0, c.g * tint.g * 2 * exposure), 1 / 2.2f), Mathf.Pow(Mathf.Max(0, c.b * tint.b * 2 * exposure), 1 / 2.2f), 1);

                }
            var tex = new Texture2D(n, n, TextureFormat.RGB24, false);

            tex.SetPixels(colors);

            tex.Apply();

            paths[side] = "Highlands/Textures/sky_" + side + ".png";

            File.WriteAllBytes(Root + "Assets/" + paths[side], tex.EncodeToPNG());

            UnityEngine.Object.DestroyImmediate(tex);

        }
        return "sky={enabled=true,faces={" + string.Join(",", new[] { paths[5], paths[4], paths[2], paths[3], paths[1], paths[0] }.Select(Q)) + "}," + AmbientFaces() + "},\n";

    }
    // 仅修正法线PNG，不重写颜色、材质、模型、场景和原有统计。
    public static void RefreshNormals()
    {
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
            var previous = JsonUtility.FromJson<Report>(File.ReadAllText(Out + "export-report.json"));
            var records = previous.materials.ToDictionary(x => x.colorTexture);
            foreach (var path in Directory.GetFiles(SourceRoot + Package + "Shaders", "*.shader.meta"))
            {
                var match = Regex.Match(File.ReadAllText(path), @"guid: ([a-f0-9]+)");
                if (match.Success)
                {
                    shaderNames[match.Groups[1].Value] = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(path));
                }
            }
            var scene = EditorSceneManager.OpenScene(Package + "Demo/Demo.unity", OpenSceneMode.Single);
            var roots = scene.GetRootGameObjects();
            foreach (var root in roots)
            {
                excluded.UnionWith(ExcludeLODs(root));
            }
            var visited = new HashSet<string>();
            int slots = 0;
            foreach (var renderer in roots.SelectMany(r => r.GetComponentsInChildren<Renderer>(true)))
            {
                if (!renderer.gameObject.activeInHierarchy || !renderer.enabled || excluded.Contains(renderer) || renderer is ParticleSystemRenderer)
                {
                    continue;
                }
                var scale = renderer.localToWorldMatrix.lossyScale;
                if (Mathf.Abs(scale.x * scale.y * scale.z) < 1e-10f)
                {
                    continue;
                }
                var filter = renderer.GetComponent<MeshFilter>();
                Mesh mesh = filter ? filter.sharedMesh : (renderer as SkinnedMeshRenderer)?.sharedMesh;
                if (!mesh)
                {
                    continue;
                }
                var materials = renderer.sharedMaterials;
                string key = Id(mesh) + "|" + string.Join("|", materials.Select(Id));
                if (!visited.Add(key))
                {
                    continue;
                }
                string id = Hash(key);
                for (int sub = 0; sub < mesh.subMeshCount; sub++)
                {
                    var mat = materials.Length > 0 ? materials[Math.Min(sub, materials.Length - 1)] : null;
                    string texture = "Highlands/Textures/" + id + "_" + sub + ".png";
                    if (!records.TryGetValue(texture, out var record))
                    {
                        throw new InvalidOperationException("Normal refresh material missing: " + texture);
                    }
                    string output = ExportNormal(new Props(mat), mesh, sub, id + "_" + sub, record);
                    if (output != record.normalTexture)
                    {
                        throw new InvalidOperationException("Normal refresh would change material binding: " + texture);
                    }
                    slots++;
                }
                Debug.Log("HIGHLANDS_NORMAL_MODEL " + visited.Count);
            }
            if (slots != previous.exportedMaterials || report.normalMaterials != previous.normalMaterials)
            {
                throw new InvalidOperationException("Normal refresh count mismatch: " + slots + "/" + report.normalMaterials);
            }
            Debug.Log("HIGHLANDS_NORMAL_REFRESH_SUCCESS models=" + visited.Count + " slots=" + slots + " normals=" + report.normalMaterials);
        }
        catch (Exception e)
        {
            Debug.LogException(e);
            EditorApplication.Exit(1);
        }
    }
    // 只刷新既有颜色 PNG；材质、法线、模型、场景和报告均保持原样。
    public static void RefreshColors()
    {
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;
            var previous = JsonUtility.FromJson<Report>(File.ReadAllText(Out + "export-report.json"));
            var records = previous.materials.ToDictionary(x => x.colorTexture);
            foreach (var path in Directory.GetFiles(SourceRoot + Package + "Shaders", "*.shader.meta"))
            {
                var match = Regex.Match(File.ReadAllText(path), @"guid: ([a-f0-9]+)");
                if (match.Success)
                {
                    shaderNames[match.Groups[1].Value] = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(path));
                }
            }
            var scene = EditorSceneManager.OpenScene(Package + "Demo/Demo.unity", OpenSceneMode.Single);
            var roots = scene.GetRootGameObjects();
            foreach (var root in roots)
            {
                excluded.UnionWith(ExcludeLODs(root));
            }
            var visited = new HashSet<string>();
            int slots = 0;
            foreach (var renderer in roots.SelectMany(r => r.GetComponentsInChildren<Renderer>(true)))
            {
                if (!renderer.gameObject.activeInHierarchy || !renderer.enabled || excluded.Contains(renderer) || renderer is ParticleSystemRenderer)
                {
                    continue;
                }
                var scale = renderer.localToWorldMatrix.lossyScale;
                if (Mathf.Abs(scale.x * scale.y * scale.z) < 1e-10f)
                {
                    continue;
                }
                var filter = renderer.GetComponent<MeshFilter>();
                Mesh mesh = filter ? filter.sharedMesh : (renderer as SkinnedMeshRenderer)?.sharedMesh;
                if (!mesh)
                {
                    continue;
                }
                var materials = renderer.sharedMaterials;
                string key = Id(mesh) + "|" + string.Join("|", materials.Select(Id));
                if (!visited.Add(key))
                {
                    continue;
                }
                string id = Hash(key);
                for (int sub = 0; sub < mesh.subMeshCount; sub++)
                {
                    var mat = materials.Length > 0 ? materials[Math.Min(sub, materials.Length - 1)] : null;
                    string texture = "Highlands/Textures/" + id + "_" + sub + ".png";
                    if (!records.TryGetValue(texture, out var record))
                    {
                        throw new InvalidOperationException("Color refresh material missing: " + texture);
                    }
                    string output = Bake(mat, mesh, sub, id + "_" + sub, out float cutoff);
                    if (output != record.colorTexture || Mathf.Abs(cutoff - record.alphaCutoff) > 1e-6f)
                    {
                        throw new InvalidOperationException("Color refresh would change material binding: " + texture);
                    }
                    slots++;
                }
                Debug.Log("HIGHLANDS_COLOR_MODEL " + visited.Count);
            }
            if (slots != previous.exportedMaterials || report.exportedMaterials != previous.exportedMaterials)
            {
                throw new InvalidOperationException("Color refresh count mismatch: " + slots + "/" + report.exportedMaterials);
            }
            int terrains = 0;
            foreach (var terrain in roots.SelectMany(r => r.GetComponentsInChildren<Terrain>()))
                BakeTerrainColor(terrain.terrainData, "terrain_" + terrains++);
            if (terrains != previous.terrainCount)
                throw new InvalidOperationException("Color refresh terrain count mismatch: " + terrains);
            Debug.Log("HIGHLANDS_COLOR_REFRESH_SUCCESS models=" + visited.Count + " slots=" + slots + " terrains=" + terrains);
        }
        catch (Exception e)
        {
            Debug.LogException(e);
            EditorApplication.Exit(1);
        }
    }
    public static void Run()
    {
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;

            Directory.CreateDirectory(Out + "Models");
            Directory.CreateDirectory(Out + "Materials");
            foreach (var path in Directory.GetFiles(SourceRoot + Package + "Shaders", "*.shader.meta"))
            {
                var match = Regex.Match(File.ReadAllText(path), @"guid: ([a-f0-9]+)");
                if (match.Success)
                {
                    shaderNames[match.Groups[1].Value] = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(path));
                }
            }

            Directory.CreateDirectory(Out + "Textures");

            Directory.CreateDirectory(Out + "Terrain");

            foreach (var path in Directory.GetFiles(SourceRoot + Package, "*.meta", SearchOption.AllDirectories))
            {
                var match = Regex.Match(File.ReadAllText(path), @"guid: ([a-f0-9]+)");
                if (match.Success)
                {
                    sourceTexturePaths[match.Groups[1].Value] = path.Substring(SourceRoot.Length).Replace('\\', '/').Substring(0, path.Length - SourceRoot.Length - 5);
                }
            }
            Debug.Log("HIGHLANDS_EXPORT_BEGIN");

            var scene = EditorSceneManager.OpenScene(Package + "Demo/Demo.unity", OpenSceneMode.Single);

            report.sourceScene = (Environment.GetEnvironmentVariable("HIGHLANDS_SOURCE_PROJECT") ?? "D:/Projects/PrototypeScene").TrimEnd('/', '\\') + "/" + Package + "Demo/Demo.unity";

            report.sourceSceneSHA256 = SourceHash(report.sourceScene);

            report.unityVersion = Application.unityVersion;

            var roots = scene.GetRootGameObjects();

            report.sourceGameObjects = roots.Sum(r => r.GetComponentsInChildren<Transform>(true).Length);

            foreach (var root in roots)
                excluded.UnionWith(ExcludeLODs(root));

            foreach (var root in roots)
                foreach (var renderer in root.GetComponentsInChildren<Renderer>(true))
                {
                    report.sourceRenderers++;

                    if (!renderer.gameObject.activeInHierarchy)
                    {
                        report.skippedInactive++;

                        continue;

                    }
                    Renderer(renderer, renderer.localToWorldMatrix, "", excluded);

                }
            report.ordinaryRenderers = report.exportedRenderers;

            Debug.Log("HIGHLANDS_MESHES_DONE renderers=" + report.exportedRenderers + " models=" + models.Count);

            foreach (var root in roots)
                foreach (var terrain in root.GetComponentsInChildren<Terrain>())
                    Terrain(terrain);

            foreach (var landmark in roots.SelectMany(r => r.GetComponentsInChildren<Transform>(true)).Where(t => t.name.IndexOf("house", StringComparison.OrdinalIgnoreCase) >= 0 || t.name.IndexOf("camp", StringComparison.OrdinalIgnoreCase) >= 0 || t.name.IndexOf("bridge", StringComparison.OrdinalIgnoreCase) >= 0).Take(80))
            {
                var rs = landmark.GetComponentsInChildren<Renderer>();

                if (rs.Length > 0)
                {
                    var b = rs[0].bounds;

                    foreach (var r in rs)
                        b.Encapsulate(r.bounds);

                    report.landmarks.Add(landmark.name + " center=" + V(Flip(b.center)) + " size=" + V(b.size));

                }
            }
            var cameras = roots.SelectMany(r => r.GetComponentsInChildren<Camera>(true)).ToArray();

            foreach (var c in cameras)
                report.cameras.Add(c.name + " eye=" + V(Flip(c.transform.position)) + " direction=" + V(Flip(c.transform.forward)) + " fov=" + F(c.fieldOfView));

            var camera = cameras.FirstOrDefault(c => c.gameObject.activeInHierarchy) ?? cameras.FirstOrDefault();

            var eye = camera ? Flip(camera.transform.position) : new Vector3(0, 100, 0);

            var direction = camera ? Flip(camera.transform.forward) : new Vector3(0, -.3f, -1);

            string projection = "camera={eye=" + V(eye) + ",direction=" + V(direction) + ",fov=" + F(camera ? camera.fieldOfView : 50) + ",near=0.1,far=3000},\n";

            var sun = roots.SelectMany(r => r.GetComponentsInChildren<Light>()).FirstOrDefault(l => l.type == LightType.Directional);

            if (sun)
            {
                var d = Flip(sun.transform.forward);

                // Unity Standard 为兼容旧灯光，直射漫反射不除 PI；当前核心的 PBR 会除 PI，所以导入时转换强度单位。
                // 源项目启用了 Linear Intensity：灯光 Color 先从 sRGB 转到线性，不能把面板数值直接当能量。
                var lightColor = sun.color.linear;
                actors.Add(Node("Highlands Sun", Vector3.zero, Quaternion.identity, Vector3.one, "{type=\"directional\",color=" + V(new Vector3(lightColor.r, lightColor.g, lightColor.b)) + ",direction=" + V(d) + ",intensity=" + F(sun.intensity * Mathf.PI) + ",attenuation={1,0,0}}"));

            }
            string lua = "-- 由隔离 Unity 编辑器读取原 Demo 与 prefab overrides 后离线导出。\nreturn {version=1,\n" + projection + Sky() + "actors={\n" + string.Join("", actors) + "}}\n";

            File.WriteAllText(Root + "Assets/Scenes/Highlands.lua", lua, new UTF8Encoding(false));

            report.terrainTreeRenderers = report.exportedRenderers - report.ordinaryRenderers;

            report.uniqueModels = models.Count;

            report.actors = actors.Count;

            report.limitations.Add("Skipped controller/scripts, audio, colliders, GI, reflection/light probes, particles, post-processing, wind, water waves, fog and emission effects. One original directional sun retained; no new rendering features.");

            report.limitations.Add("Static material RGB and foliage UV/local-height second color baked; world-noise second color approximated with 0.5. Cutout alpha uses the source image; true transparent materials also multiply BaseColor alpha. Overlapping UV islands share the first matching local-height mask, with bounded raster work per material; coverage RGB/base+coverage normals are statically baked with documented local-space approximation; vertex AO/detail normals are not reproduced. Terrain detail grass prototypes are counted but not generated; terrain treeInstances exported.");

            File.WriteAllText(Out + "export-report.json", JsonUtility.ToJson(report, true));

            Debug.Log("HIGHLANDS_EXPORT_SUCCESS " + JsonUtility.ToJson(report));

        }
        catch (Exception e)
        {
            Debug.LogException(e);
            EditorApplication.Exit(1);
        }
    }
}



