#version 430 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

in vec2 TexCoords;

// Bo in d light
struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 diffuse;
	vec3 specular;
};
struct DirLight {
	vec3 direction;

	vec3 diffuse;
	vec3 specular;
};

uniform DirLight dirLight;
uniform PointLight pointLights[5];
uniform vec3 viewPos;

out vec4 FragColor;

void main()
{ 
	vec3 fragPos = texture(gPosition, TexCoords).rgb;
	vec3 normal = texture(gNormal, TexCoords).rgb;
	vec3 albedo = texture(gAlbedoSpec, TexCoords).rgb;

	vec3 lighting = albedo * 0.1;
	vec3 viewDir = normalize(viewPos - fragPos);
	for (int i = 0; i < 5; i++)
	{
		vec3 lightDir = normalize(pointLights[i].position - fragPos);
		vec3 diffuse = max(dot(normal, lightDir), 0.0) * albedo * pointLights[i].diffuse;

		vec3 halfDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(normal, halfDir), 0.0), 16.0);
		vec3 specular = pointLights[i].specular * spec;

		float distance = length(pointLights[i].position - fragPos);
		float attenuation = 1.0 / (1.0 + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
		diffuse *= attenuation;
		specular *= attenuation;
		lighting += diffuse + specular;
	}


	{
		vec3 lightDir = -dirLight.direction;
		vec3 diffuse = max(dot(normal, lightDir), 0.0) * albedo * dirLight.diffuse;

		vec3 halfDir = normalize(lightDir + viewDir);
		float spec = pow(max(dot(normal, halfDir), 0.0), 16.0);
		vec3 specular = dirLight.specular * spec;

		lighting += diffuse + specular;
	}

	FragColor = vec4(lighting, 1.0);

}