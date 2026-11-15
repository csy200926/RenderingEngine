#version 420

in vec3 vp;

uniform mat4 WorldToView_Matrix;
uniform mat4 Projective_Matrix;

// per instance
uniform mat4 object2World_matrix;

out vec3 texcoords;
out vec3 worldPos;

void main() {

	worldPos = vp;

	texcoords = vp;

	//Disable camera translation
	vec4 rotated = WorldToView_Matrix * vec4(vp, 1.0);
	//rotated.w = 1.0;

	gl_Position = Projective_Matrix * rotated;
}
