#version 120

//DEFINES

uniform mat4 positionMatrix;
uniform mat3 normalMatrix;

in vec4 vertPos;
in vec4 vertTexCoord;
in vec3 vertNormal;
in vec4 vertColor;

varying vec4 texCoord;
varying vec4 vTexCoord;
varying vec3 normal;
varying vec3 vNormal;
varying vec4 color;
varying vec4 vColor;

void main() {
	gl_Position = positionMatrix * vertPos;
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(normalMatrix * vertNormal);
	vColor = color = vertColor;
}

