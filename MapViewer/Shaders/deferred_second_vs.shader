#version 430 core

layout (location = 0) in vec3 position;


out vec2 TexCoords;


uniform mat4 ModelToWorld_Matrix;   
uniform mat4 WorldToView_Matrix;                                                     
uniform mat4 Projective_Matrix;    

void main()
{
	TexCoords = (position.xy + vec2(1, 1)) / 2.0;;
	gl_Position = vec4(position,1);//Projective_Matrix * WorldToView_Matrix * ModelToWorld_Matrix * vec4(position, 1.0);
}