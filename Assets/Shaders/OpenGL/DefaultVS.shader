#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoords;
uniform mat4 ModelToWorld_Matrix;
uniform mat4 WorldToView_Matrix;
uniform mat4 Projective_Matrix;
uniform mat4 Normal_Matrix;
uniform vec4 materialParams;
uniform mat4 LightViewProjection_Matrix;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 CubeDirection;
void main()
{
    vec4 world = ModelToWorld_Matrix * vec4(position,1.0);
    gl_Position = Projective_Matrix * WorldToView_Matrix * world;
    if (materialParams.w > 2.5) gl_Position = LightViewProjection_Matrix * world;
    else if (materialParams.w > 1.5) gl_Position.z = gl_Position.w;
    FragPos = world.xyz;
    Normal = mat3(Normal_Matrix) * normal;
    TexCoords = texCoords;
    CubeDirection = position;
}
