#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D screenTexture;

void main()
{ 
    color = vec4(1,1,1,1);//texture(screenTexture, TexCoords);
}