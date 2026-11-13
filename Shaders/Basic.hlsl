// Vulkan 编译成 SPIR-V，DX12 仍用这一份代码。
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
// Model 和 Color 直接跟随绘制命令。
[[vk::push_constant]] ObjectData Object;
#else
// 对应 DX12 的 Root Constants。
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
    // CPU 投影还是 [-1,1]，这里换到 [0,1]。Vulkan 的 Y 在编译时翻。
    output.position.z = (output.position.z+output.position.w)*0.5;
    output.worldPosition = world.xyz;
    // Demo 只有等比缩放；以后支持任意缩放时再传 inverse-transpose 法线矩阵。
    output.normal = mul((float3x3)Object.Model,input.normal);
    output.uv = input.uv;
    return output;
}
float4 PSMain(VertexOutput input) : SV_TARGET
{
    // 漫反射加一点高光。
    float3 N = normalize(input.normal), L = normalize(-SunDirection.xyz);
    float3 V = normalize(CameraPosition.xyz-input.worldPosition);
    float diffuse = max(dot(N,L),0);
    float specular = pow(max(dot(N,normalize(L+V)),0),32)*0.12;
    // 暂时用棋盘格，看一下 UV。
    float checker = fmod(floor(input.uv.x*8)+floor(input.uv.y*8),2);
    float3 color = Object.Color.rgb;
    color *= lerp(0.78,1.0,checker);
    color *= 0.22+0.78*diffuse;

    // color = Object.Color.rgb*lerp(0.78,1.0,checker)*(0.22+0.78*diffuse)+specular;
    color += specular;
    return float4(pow(max(color,0),1.0/2.2),1);
}
