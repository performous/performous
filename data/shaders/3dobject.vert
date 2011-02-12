#version 120

in vec4 vertPos;
in vec4 vertTexCoord;
in vec3 vertNormal;

varying float bogus;

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec4 texCoord;
varying vec3 normal;

void main() {
	bogus = 0.0;
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	texCoord = vertTexCoord;
	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
	normal = normalize(gl_NormalMatrix * vertNormal);
}
