#version 420 core                

// Output
layout (location = 0) out vec4 color;

// Input from vertex shader
in VS_OUT
{
    vec3 worldPos;
    vec3 worldNormal;
} fs_in;

struct DirLight {
	vec3 direction;

	vec3 diffuse;
	vec3 specular;
};

uniform DirLight dirLight;
uniform vec3 viewPos;

void main(void)
{
	vec3 worldNormal = normalize(fs_in.worldNormal);
	vec3 worldPos = fs_in.worldPos;

	vec3 lightDirection = -dirLight.direction;

	// Diffuse
	float NDL = max(dot(worldNormal,lightDirection),0.0);
	vec3 diffuse = NDL * vec3(1,1,0);

	// Specular
	vec3 reflectDir = normalize(reflect(-lightDirection,worldNormal));
	vec3 viewDir = normalize(viewPos - worldPos);
	vec3 specular = vec3(1,1,1) * pow( max(dot(viewDir,reflectDir),0.0), 5.0 );

	// Final
	color = vec4(diffuse + specular,1.0);
}
