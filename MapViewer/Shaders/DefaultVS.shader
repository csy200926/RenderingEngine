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
out vec2 TexCoords;



// Position of light
uniform vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
    // Calculate view-space coordinate
    vec4 P = Projective_Matrix * WorldToView_Matrix * ModelToWorld_Matrix * vec4(position,1.0);
	TexCoords = texCoords;
}
