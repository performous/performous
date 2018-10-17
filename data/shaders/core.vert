#version 330 core

//DEFINES

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTexCoord;
layout(location = 2) in vec3 vertNormal;
layout(location = 3) in vec4 vertColor;

out vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
} vertex;

void main() {
	//const vec3 lightPos = vec3(0.0, -2.94, 1.0);
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);

	vec4 posEye = mvMatrix * vec4(vertPos, 1.0); // Vertex position in eye space
	gl_Position = projMatrix * posEye; // Vertex position in normalized device coordinates
	vertex.lightDir = lightPos - posEye.xyz / posEye.w; // Light position relative to vertex
	vertex.texCoord = vertTexCoord;
	vertex.normal = normalize(normalMatrix * vertNormal);
	vertex.color = vertColor;
}

