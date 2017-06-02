#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal; 
layout (location = 2) in vec2 texCoords;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 ModelToWorld_Matrix;   
uniform mat4 WorldToView_Matrix;                                                     
uniform mat4 Projective_Matrix;    

void main()
{
    vec4 worldPos = ModelToWorld_Matrix * vec4(position, 1.0);

    FragPos = worldPos.xyz; 
    TexCoords = texCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(ModelToWorld_Matrix)));
    Normal = normalMatrix * normal;

    gl_Position = Projective_Matrix * WorldToView_Matrix * worldPos;
}