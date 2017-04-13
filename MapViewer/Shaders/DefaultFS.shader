#version 420 core                

// Output
out vec4 color;

// Input from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Material properties
uniform sampler2D texture_diffuse1;

// Dai right light
struct DirLight {
    vec3 direction;
	
    vec3 diffuse;
    vec3 specular;
};

// Bo in d light
struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 diffuse;
    vec3 specular;
}; 

struct Spot {
    vec3  position;
    vec3  direction;
    float cutOff;

    vec3 diffuse;
    vec3 specular;
};  

uniform PointLight pointLights[5];

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  

uniform vec3 viewPos;
uniform DirLight dirLight;

void main(void)
{
    // Properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

	vec3 result = CalcDirLight(dirLight, norm, viewDir);

	const int NR_POINT_LIGHTS = 5;

    for(int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);   

	color = vec4(result, 1.0);
}

// Calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(lightDir, normal);
    float spec = pow(max(dot(-viewDir, reflectDir), 0.0), 2);
    // Combine results
	vec3 albedo = vec3(texture(texture_diffuse1, TexCoords));

    vec3 diffuse = light.diffuse * diff *  albedo;
    vec3 specular = light.specular * spec * albedo;// * vec3(1,1,1);
    return (diffuse + specular);
}

// Calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);
    // Attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Combine results
	vec3 albedo = vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * albedo;// * vec3(1,1,1);
    diffuse  *= attenuation;
    specular *= attenuation;
    return (diffuse + specular);
} 