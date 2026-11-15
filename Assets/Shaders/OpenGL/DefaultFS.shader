#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 CubeDirection;
out vec4 FragColor;
uniform sampler2D basic_texture;
uniform samplerCube irradiance_texture;
uniform samplerCube cube_texture;
uniform sampler2D opacity_texture;
uniform sampler2D normal_texture;
uniform sampler2D shadow_texture;
uniform mat4 LightViewProjection_Matrix;
uniform vec4 shadowParams;
uniform vec4 viewPosition;
uniform vec4 materialColor;
uniform vec4 materialParams;
uniform vec4 surfaceParams;
uniform vec4 sunDirection;
uniform vec4 sunColor;
uniform vec4 pointPosition[4];
uniform vec4 pointColor[4];
uniform vec4 pointAttenuation[4];
const float PI = 3.14159265359;
// GGX, Smith and Schlick are migrated from the user's original DefaultFS.shader.
float DistributionGGX(vec3 N,vec3 H,float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N,H),0.0);
    float denom = NdotH*NdotH*(a2-1.0)+1.0;
    return a2 / max(PI*denom*denom,0.000001);
}
float GeometrySchlickGGX(float NdotV,float roughness)
{
    float r = roughness+1.0;
    float k = r*r/8.0;
    return NdotV / max(NdotV*(1.0-k)+k,0.000001);
}
float GeometrySmith(vec3 N,vec3 V,vec3 L,float roughness)
{
    return GeometrySchlickGGX(max(dot(N,V),0.0),roughness)*GeometrySchlickGGX(max(dot(N,L),0.0),roughness);
}
vec3 fresnelSchlick(float cosTheta,vec3 F0)
{
    return F0+(1.0-F0)*pow(1.0-cosTheta,5.0);
}
vec3 CalculateLight(vec3 N,vec3 V,vec3 L,vec3 radiance,vec3 albedo,vec3 F0,float metallic,float roughness)
{
    vec3 H = normalize(V+L);
    vec3 F = fresnelSchlick(max(dot(H,V),0.0),F0);
    float NDF = DistributionGGX(N,H,roughness);
    float G = GeometrySmith(N,V,L,roughness);
    vec3 specular = NDF*G*F/(4.0*max(dot(N,V),0.0)*max(dot(N,L),0.0)+0.001);
    vec3 kD = (1.0-F)*(1.0-metallic);
    return (kD*albedo/PI+specular)*radiance*max(dot(N,L),0.0);
}
// 九次独立深度比较再平均，柔化阴影；不把不同深度先线性混在一起。
float SunVisibility(vec3 worldPosition, vec3 geometricNormal)
{
    if (shadowParams.w < 0.5) return 1.0;
    vec4 projected = LightViewProjection_Matrix*vec4(worldPosition,1.0);
    vec3 p = projected.xyz/projected.w;
    if (any(greaterThanEqual(abs(p),vec3(1.0)))) return 1.0;
    vec2 uv = p.xy*0.5+0.5;
    float depth = p.z*0.5+0.5;
    // 按接收面的斜率修正九个采样点的深度，避免斜面被自己的邻近像素遮挡。
    vec2 dx = dFdx(uv), dy = dFdy(uv);
    float dzdx = dFdx(depth), dzdy = dFdy(depth);
    float determinant = dx.x*dy.y-dx.y*dy.x;
    vec2 gradient = abs(determinant) > 0.000000000001 ?
        vec2(dzdx*dy.y-dzdy*dx.y,dx.x*dzdy-dy.x*dzdx)/determinant : vec2(0.0);
    float slope = 1.0-clamp(dot(normalize(geometricNormal),normalize(-sunDirection.xyz)),0.0,1.0);
    float bias = shadowParams.y*(1.0+3.0*slope);
    int size = int(1.0/shadowParams.x+0.5);
    ivec2 pixel = ivec2(uv*float(size));
    float visible = 0.0;
    for (int y=-1;y<=1;++y)
        for (int x=-1;x<=1;++x)
        {
            ivec2 samplePixel = clamp(pixel+ivec2(x,y),ivec2(0),ivec2(size-1));
            float stored = texelFetch(shadow_texture,samplePixel,0).r;
            float receiverDepth = depth+dot(gradient,(vec2(samplePixel)+0.5)/float(size)-uv);
            visible += receiverDepth-bias <= stored ? 1.0 : 0.0;
        }
    float fade = clamp((1.0-max(abs(p.x),abs(p.y)))/0.08,0.0,1.0);
    return 1.0-(1.0-visible/9.0)*shadowParams.z*fade;
}
void main()
{
    if (materialParams.w > 2.5)
    {
        float alpha = texture(basic_texture,TexCoords).a*materialColor.a;
        if (surfaceParams.y > 0.5) alpha *= texture(opacity_texture,TexCoords).r;
        if (alpha < surfaceParams.x) discard;
        FragColor = vec4(gl_FragCoord.z,0.0,0.0,1.0);
        return;
    }
    if (materialParams.w > 1.5)
    {
        FragColor = vec4(texture(cube_texture,normalize(CubeDirection)).rgb,1.0);
        return;
    }
    vec4 texel = texture(basic_texture,TexCoords)*materialColor;
    if (materialParams.w > 0.5)
    {
        if (texel.a < 0.005) discard;
        FragColor = texel;
        return;
    }
    float alpha = texel.a;
    if (surfaceParams.y > 0.5) alpha *= texture(opacity_texture,TexCoords).r;
    if (alpha < surfaceParams.x) discard;
    vec3 albedo = pow(texture(basic_texture,TexCoords).rgb,vec3(2.2))*materialColor.rgb;
    float metallic = materialParams.x;
    float roughness = materialParams.y;
    vec3 N = normalize(Normal);
    if (surfaceParams.z > 0.5)
    {
        vec3 dp1=dFdx(FragPos),dp2=dFdy(FragPos);
        vec2 duv1=dFdx(TexCoords),duv2=dFdy(TexCoords);
        float determinant=duv1.x*duv2.y-duv1.y*duv2.x;
        if (abs(determinant)>0.00000001)
        {
            vec3 T=normalize((dp1*duv2.y-dp2*duv1.y)/determinant);
            vec3 B=normalize((-dp1*duv2.x+dp2*duv1.x)/determinant);
            vec3 tangentNormal=texture(normal_texture,TexCoords).rgb*2.0-1.0;
            N=normalize(mat3(T,B,N)*tangentNormal);
        }
    }
    vec3 V = normalize(viewPosition.xyz-FragPos);
    vec3 F0 = mix(vec3(0.04),albedo,metallic);
    vec3 Lo = CalculateLight(N,V,normalize(-sunDirection.xyz),sunColor.rgb*SunVisibility(FragPos,Normal),albedo,F0,metallic,roughness);
    for (int i=0;i<4;++i)
    {
        vec3 offset = pointPosition[i].xyz-FragPos;
        float distance = max(length(offset),0.0001);
        vec3 attenuation = pointAttenuation[i].xyz;
        float factor = 1.0/max(attenuation.x+attenuation.y*distance+attenuation.z*distance*distance,0.0001);
        Lo += CalculateLight(N,V,offset/distance,pointColor[i].rgb*factor,albedo,F0,metallic,roughness);
    }
    // 环境漫反射不随视角归零，否则法线偏转时会出现纯黑斑。
    vec3 kS = F0;
    vec3 irradiance = pow(texture(irradiance_texture,N).rgb,vec3(2.2));
    vec3 color = ((1.0-kS)*(1.0-metallic)*irradiance*albedo)*materialParams.z+Lo;
    color = color/(color+vec3(1.0));
    FragColor = vec4(pow(max(color,vec3(0.0)),vec3(1.0/2.2)),alpha);
}
