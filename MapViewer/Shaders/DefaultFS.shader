#version 420 core                

// Output
out vec4 color;

// Input from vertex shader
in vec2 TexCoords;

// Material properties
uniform sampler2D texture_diffuse1;


void main(void)
{
	color = vec4(texture(texture_diffuse1,TexCoords));
}
