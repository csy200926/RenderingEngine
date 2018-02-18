#version 420 core

// Per-vertex inputs
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal; 

// Matrices we'll need
uniform mat4 WorldToView_Matrix;                                             
uniform mat4 ModelToWorld_Matrix;                                           
uniform mat4 Projective_Matrix;    


// Inputs from vertex shader
out VS_OUT
{
    vec3 worldPos;
    vec3 worldNormal;
} vs_out;


void main(void)
{
	vs_out.worldPos = (ModelToWorld_Matrix * position).xyz;
	vs_out.worldNormal = (ModelToWorld_Matrix * vec4(normal,1.0)).xyz;

	gl_Position = Projective_Matrix * WorldToView_Matrix * ModelToWorld_Matrix * position;;
}
