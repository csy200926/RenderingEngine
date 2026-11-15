#version 430 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D screenTexture;

const float offset = 1.0 / 300;  
const vec2 offsets[9] = vec2[](
		vec2(-offset, offset),  // top-left
		vec2(0.0f,    offset),  // top-center
		vec2(offset,  offset),  // top-right
		vec2(-offset, 0.0f),    // center-left
		vec2(0.0f,    0.0f),    // center-center
		vec2(offset,  0.0f),    // center-right
		vec2(-offset, -offset), // bottom-left
		vec2(0.0f,    -offset), // bottom-center
		vec2(offset,  -offset)  // bottom-right    
    );

const float kernel_Sharpen[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

const float kernel_Blur[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
);

const float kernel_Edge[9] = float[](
        1, 1, 1,
        1,-8, 1,
        1, 1, 1
);
void main()
{ 
    color = texture(screenTexture, TexCoords);
	float gamma = 2.2;
	color.rgb = pow(color.rgb, vec3(1.0 / gamma));
	/*
	vec3 col = vec3(0.0);
	for(int i = 0 ; i < 9 ; i++)
	{
		col += vec3(texture(screenTexture,TexCoords + offsets[i])) * kernel_Edge[i];

	}
	color = vec4(col,1.0);
	*/

	/*Sharpen
	vec3 col = vec3(0.0);
	for(int i = 0 ; i < 9 ; i++)
	{
		col += vec3(texture(screenTexture,TexCoords + offsets[i])) * kernel_Sharpen[i];

	}
	color = vec4(col,1.0);
	*/
	
	//Grayscale
	//float average = (color.r + color.g + color.b) / 3.0;
	//color = vec4(average,average,average,1.0);

	//Inversion
	//color = 1.0 - color;
}