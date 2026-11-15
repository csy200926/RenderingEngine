#version 420 core

// Per-vertex inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal; 
layout(location = 2) in vec2 texCoords;

// Matrices we'll need
uniform mat4 ModelToWorld_Matrix;   
uniform mat4 WorldToView_Matrix;                                                     
uniform mat4 Projective_Matrix;    

// Inputs from vertex shader
out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;


void main(void)
{
    // Calculate view-space coordinate
    gl_Position = Projective_Matrix * WorldToView_Matrix * ModelToWorld_Matrix * vec4(position,1.0);
	FragPos = vec3(ModelToWorld_Matrix * vec4(position,1.0f));
	Normal = mat3(transpose(inverse(ModelToWorld_Matrix))) * normal;  
	TexCoords = texCoords;
}
