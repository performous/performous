#version 120

//DEFINES

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

attribute vec4 vertPos;
attribute vec4 vertTexCoord;
attribute vec3 vertNormal;
attribute vec4 vertColor;

varying vec3 lightDir;
varying vec3 vLightDir;
varying vec4 texCoord;
varying vec4 vTexCoord;
varying vec3 normal;
varying vec3 vNormal;
varying vec4 color;
varying vec4 vColor;

void main() {
	//const vec3 lightPos = vec3(0.0, -2.94, 1.0);
	const vec3 lightPos = vec3(-10.0, 2.0, 15.0);

	vec4 posEye = mvMatrix * vertPos;  // Vertex position in eye space
	gl_Position = projMatrix * posEye;  // Vertex position in normalized device coordinates
	vLightDir = lightDir = lightPos - posEye.xyz / posEye.w;  // Light position relative to vertex
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(normalMatrix * vertNormal);
	vColor = color = vertColor;
}

