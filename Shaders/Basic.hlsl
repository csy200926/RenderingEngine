// 先读 VSMain 再读 PSMain。同一份 HLSL：DX12 编成 DXBC，Vulkan 由目录内 DXC 编成 SPIR-V。
cbuffer PerFrame : register(b0)
{
    column_major float4x4 ViewProjection;
    float4 CameraPosition;
    float4 SunDirection;
};
struct ObjectData
{
    column_major float4x4 Model;
    float4 Color;
};
#ifdef VULKAN
// 80 字节小数据直接放在命令里，不为每个方块另分配 UBO。
[[vk::push_constant]] ObjectData Object;
#else
// DX12 对应 Root Constants，同样把这 80 字节写进命令。
cbuffer PerObject : register(b1) { ObjectData Object; };
#endif
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD2;
};
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    float4 world = mul(Object.Model,float4(input.position,1));
    output.position = mul(ViewProjection,world);
    // CPU 统一使用 RH / [-1,1] 深度，两个 API 在这里转到 [0,1]。
    // Vulkan 画面 Y 翻转由编译参数 -fvk-invert-y 完成。
    output.position.z = (output.position.z+output.position.w)*0.5;
    output.worldPosition = world.xyz;
    // Demo 只有等比缩放；以后支持任意缩放时再传 inverse-transpose 法线矩阵。
    output.normal = mul((float3x3)Object.Model,input.normal);
    output.uv = input.uv;
    return output;
}
float4 PSMain(VertexOutput input) : SV_TARGET
{
    // 简单漫反射 + 小高光，让朝向和立体感可见；这里还不是 PBR 材质系统。
    float3 N = normalize(input.normal), L = normalize(-SunDirection.xyz);
    float3 V = normalize(CameraPosition.xyz-input.worldPosition);
    float diffuse = max(dot(N,L),0);
    float specular = pow(max(dot(N,normalize(L+V)),0),32)*0.12;
    // 用 UV 算棋盘格，说明插值坐标的作用，不依赖外部图片。
    float checker = fmod(floor(input.uv.x*8)+floor(input.uv.y*8),2);
    float3 color = Object.Color.rgb*lerp(0.78,1.0,checker)*(0.22+0.78*diffuse)+specular;
    return float4(pow(max(color,0),1.0/2.2),1);
}
