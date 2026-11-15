cbuffer PerDraw : register(b0)
{
    column_major float4x4 ModelToWorld_Matrix;
    column_major float4x4 WorldToView_Matrix;
    column_major float4x4 Projective_Matrix;
    column_major float4x4 Normal_Matrix;
    float4 viewPosition;
    float4 materialColor;
    float4 materialParams;
    float4 surfaceParams;
    float4 sunDirection;
    float4 sunColor;
    float4 pointPosition[4];
    float4 pointColor[4];
    float4 pointAttenuation[4];
    column_major float4x4 LightViewProjection_Matrix;
    float4 shadowParams;
};
Texture2D basic_texture : register(t0);
TextureCube irradiance_texture : register(t1);
TextureCube cube_texture : register(t2);
Texture2D opacity_texture : register(t3);
Texture2D normal_texture : register(t4);
Texture2D shadow_texture : register(t5);
SamplerState linearSampler : register(s0);
SamplerState cubeSampler : register(s1);
struct VSInput { float3 position:POSITION; float3 normal:NORMAL; float2 texCoords:TEXCOORD0; };
struct VSOutput
{
    float4 position:SV_POSITION;
    float3 FragPos:TEXCOORD0;
    float3 Normal:TEXCOORD1;
    float2 TexCoords:TEXCOORD2;
    float3 CubeDirection:TEXCOORD3;
};
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 world = mul(ModelToWorld_Matrix,float4(input.position,1));
    output.position = mul(Projective_Matrix,mul(WorldToView_Matrix,world));
    // 阴影时从太阳方向观察场景，天空仍放在相机最远处。
    if (materialParams.w > 2.5) output.position = mul(LightViewProjection_Matrix,world);
    else if (materialParams.w > 1.5) output.position.z = output.position.w;
    // CPU 使用共享 RH / OpenGL 投影，深度从 [-w,w] 转为 Vulkan 的 [0,w]。
    // 屏幕 Y 由固定离线 DXC 的 -fvk-invert-y 翻转，不改 CPU 矩阵或纹理坐标。
    output.position.z = (output.position.z+output.position.w)*0.5;
    output.FragPos = world.xyz;
    output.Normal = mul((float3x3)Normal_Matrix,input.normal);
    output.TexCoords = input.texCoords;
    output.CubeDirection = input.position;
    return output;
}
static const float PI = 3.14159265359;
// 沿用原 GLSL/DX11 PBR：GGX 法线分布、Smith 几何遮蔽、Schlick Fresnel。
// 颜色先转线性空间累加光照，最后 tonemap/gamma。环境卷积仅表示漫反射环境光。
float DistributionGGX(float3 N,float3 H,float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H),0.0);
    float denom = NdotH*NdotH*(a2-1.0)+1.0;
    return a2/max(PI*denom*denom,0.000001);
}
float GeometrySchlickGGX(float NdotV,float roughness)
{
    float r = roughness+1.0;
    float k = r*r/8.0;
    return NdotV/max(NdotV*(1.0-k)+k,0.000001);
}
float GeometrySmith(float3 N,float3 V,float3 L,float roughness)
{
    return GeometrySchlickGGX(max(dot(N,V),0.0),roughness)*GeometrySchlickGGX(max(dot(N,L),0.0),roughness);
}
float3 fresnelSchlick(float cosTheta,float3 F0)
{
    return F0+(1.0-F0)*pow(1.0-cosTheta,5.0);
}
float3 CalculateLight(float3 N,float3 V,float3 L,float3 radiance,float3 albedo,float3 F0,float metallic,float roughness)
{
    float3 H = normalize(V+L);
    float3 F = fresnelSchlick(max(dot(H,V),0.0),F0);
    float NDF = DistributionGGX(N,H,roughness);
    float G = GeometrySmith(N,V,L,roughness);
    float3 specular = NDF*G*F/(4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.001);
    float3 kD = (1.0-F)*(1.0-metallic);
    return (kD*albedo/PI+specular)*radiance*max(dot(N,L),0.0);
}
// 将世界坐标投到太阳的图中，比较九个相邻深度，柔化阴影边缘。
float SunVisibility(float3 worldPosition, float3 geometricNormal)
{
    if (shadowParams.w < 0.5) return 1;
    float4 projected = mul(LightViewProjection_Matrix,float4(worldPosition,1));
    float3 p = projected.xyz/projected.w;
    if (any(abs(p) >= 1.0)) return 1;
    // D3D/Vulkan 图片上边是第零行，光空间 Y 向上，所以这里翻 Y。
    float2 uv = float2(p.x*0.5+0.5,0.5-p.y*0.5);
    float depth = p.z*0.5+0.5;
    // 邻近像素在斜面上本来就有不同深度，按接收面梯度修正，减少岩石、地面的条纹。
    float2 dx = ddx(uv), dy = ddy(uv);
    float dzdx = ddx(depth), dzdy = ddy(depth);
    float determinant = dx.x*dy.y-dx.y*dy.x;
    float2 gradient = abs(determinant) > 0.000000000001 ?
        float2(dzdx*dy.y-dzdy*dx.y,dx.x*dzdy-dy.x*dzdx)/determinant : float2(0,0);
    float slope = 1.0-saturate(dot(normalize(geometricNormal),normalize(-sunDirection.xyz)));
    float bias = shadowParams.y*(1.0+3.0*slope);
    int size = int(1.0/shadowParams.x+0.5);
    int2 pixel = int2(uv*size);
    float visible = 0;
    for (int y=-1;y<=1;++y)
        for (int x=-1;x<=1;++x)
        {
            int2 samplePixel = clamp(pixel+int2(x,y),int2(0,0),int2(size-1,size-1));
            float stored = shadow_texture.Load(int3(samplePixel,0)).r;
            float receiverDepth = depth+dot(gradient,(float2(samplePixel)+0.5)/size-uv);
            visible += receiverDepth-bias <= stored ? 1.0 : 0.0;
        }
    // 覆盖范围的边缘逐渐退出，避免一条明显的阴影矩形边界。
    float fade = saturate((1.0-max(abs(p.x),abs(p.y)))/0.08);
    return 1.0-(1.0-visible/9.0)*shadowParams.z*fade;
}
float4 PSMain(VSOutput input):SV_TARGET
{
    if (materialParams.w > 2.5)
    {
        // 叶片与草仍按原贴图剪裁，不能让整块四边形投下方形阴影。
        float alpha = basic_texture.Sample(linearSampler,input.TexCoords).a*materialColor.a;
        if (surfaceParams.y > 0.5) alpha *= opacity_texture.Sample(linearSampler,input.TexCoords).r;
        if (surfaceParams.x > 0.0) clip(alpha-surfaceParams.x);
        return float4(input.position.z,0,0,1);
    }
    if (materialParams.w > 1.5) return float4(cube_texture.Sample(cubeSampler,normalize(input.CubeDirection)).rgb,1);
    float4 texel = basic_texture.Sample(linearSampler,input.TexCoords)*materialColor;
    if (materialParams.w > 0.5)
    {
        clip(texel.a-0.005);
        return texel;
    }
    float alpha = texel.a;
    if (surfaceParams.y > 0.5) alpha *= opacity_texture.Sample(linearSampler,input.TexCoords).r;
    if (surfaceParams.x > 0.0) clip(alpha-surfaceParams.x);
    float3 albedo = pow(basic_texture.Sample(linearSampler,input.TexCoords).rgb,2.2)*materialColor.rgb;
    float metallic = materialParams.x;
    float roughness = materialParams.y;
    float3 N = normalize(input.Normal);
    if (surfaceParams.z > 0.5)
    {
        float3 dp1=ddx(input.FragPos),dp2=ddy(input.FragPos);
        float2 duv1=ddx(input.TexCoords),duv2=ddy(input.TexCoords);
        float determinant=duv1.x*duv2.y-duv1.y*duv2.x;
        if (abs(determinant)>0.00000001)
        {
            float3 T=normalize((dp1*duv2.y-dp2*duv1.y)/determinant);
            float3 B=normalize((-dp1*duv2.x+dp2*duv1.x)/determinant);
            float3 tangentNormal=normal_texture.Sample(linearSampler,input.TexCoords).rgb*2.0-1.0;
            N=normalize(T*tangentNormal.x+B*tangentNormal.y+N*tangentNormal.z);
        }
    }
    float3 V = normalize(viewPosition.xyz-input.FragPos);
    float3 F0 = lerp(float3(0.04,0.04,0.04),albedo,metallic);
    float3 Lo = CalculateLight(N,V,normalize(-sunDirection.xyz),sunColor.rgb*SunVisibility(input.FragPos,input.Normal),albedo,F0,metallic,roughness);
    for (int i=0;i<4;++i)
    {
        float3 offset = pointPosition[i].xyz-input.FragPos;
        float distance = max(length(offset),0.0001);
        float3 attenuation = pointAttenuation[i].xyz;
        float factor = 1.0/max(attenuation.x+attenuation.y*distance+attenuation.z*distance*distance,0.0001);
        Lo += CalculateLight(N,V,offset/distance,pointColor[i].rgb*factor,albedo,F0,metallic,roughness);
    }
    // 环境漫反射用基础反射率保留能量；视角Fresnel只用于直射高光，避免背向扰动法线把环境光清零。
    float3 kS = F0;
    float3 irradiance = pow(irradiance_texture.Sample(cubeSampler,N).rgb,2.2);
    float3 color = ((1.0-kS)*(1.0-metallic)*irradiance*albedo)*materialParams.z+Lo;
    color = color/(color+1.0);
    return float4(pow(max(color,0.0),1.0/2.2),alpha);
}
