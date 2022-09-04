#version 330 core

//DEFINES

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTexCoord;
layout(location = 2) in vec3 vertNormal;
layout(location = 3) in vec4 vertColor;

layout (std140) uniform shaderMatrices {
	mat4 projMatrix;
	mat4 mvMatrix;
	mat4 normalMatrix;
	mat4 colorMatrix;
};

out vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
} vertex;

void main() {
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);
	vec4 posEye = mvMatrix * vec4(vertPos, 1.0); // Vertex position in eye space
	gl_Position = projMatrix * posEye; // Vertex position in normalized device coordinates
	vertex.lightDir = lightPos - posEye.xyz / posEye.w; // Light position relative to vertex
	vertex.texCoord = vertTexCoord;
	vertex.normal = normalize(mat3(normalMatrix) * vertNormal);
	vertex.color = vertColor;
}
