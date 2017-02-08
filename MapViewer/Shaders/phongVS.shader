#version 420 core

// Per-vertex inputs
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv; 
layout(location = 2) in vec4 color;
layout(location = 3) in vec3 normal;

// Matrices we'll need
uniform mat4 WorldToView_Matrix;                                             
uniform mat4 ModelToWorld_Matrix;                                           
uniform mat4 Projective_Matrix;    


// Inputs from vertex shader
out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
	vec3 color;
	vec2 uv;
} vs_out;

// Position of light
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
    // Calculate view-space coordinate
    vec4 P = WorldToView_Matrix * ModelToWorld_Matrix * position;

    // Calculate normal in view-space
    vs_out.N = mat3(WorldToView_Matrix) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector
    vs_out.V = -P.xyz;

    // Calculate the clip-space position of each vertex
    gl_Position = Projective_Matrix * P;

	vs_out.color = color.rgb;
}
