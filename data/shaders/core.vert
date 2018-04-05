#version 330 core

//DEFINES

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

in vec3 vertPos;
in vec2 vertTexCoord;
in vec3 vertNormal;
in vec4 vertColor;

out vec3 lightDir;
out vec3 vLightDir;
out vec2 texCoord;
out vec2 vTexCoord;
out vec3 normal;
out vec3 vNormal;
out vec4 color;
out vec4 vColor;

void main() {
	//const vec3 lightPos = vec3(0.0, -2.94, 1.0);
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);

	vec4 posEye = mvMatrix * vec4(vertPos, 1.0);  // Vertex position in eye space
	gl_Position = projMatrix * posEye;  // Vertex position in normalized device coordinates
	vLightDir = lightDir = lightPos - posEye.xyz / posEye.w;  // Light position relative to vertex
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(normalMatrix * vertNormal);
	vColor = color = vertColor;
}

