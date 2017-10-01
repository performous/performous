#version 330

//DEFINES

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTexCoord;
layout(location = 2) in vec3 vertNormal;
layout(location = 3) in vec4 vertColor;

out vec3 vLightDir;
out vec2 vTexCoord;
out vec3 vNormal;
out vec4 vColor;

void main() {
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);

	vec4 posEye = mvMatrix * vec4(vertPos, 1.0);  // Vertex position in eye space
	gl_Position = projMatrix * posEye;  // Vertex position in normalized device coordinates
	vLightDir = lightPos - posEye.xyz / posEye.w;  // Light position relative to vertex
	vTexCoord = vertTexCoord;
	vNormal = normalize(normalMatrix * vertNormal);
	vColor = vertColor;
}
