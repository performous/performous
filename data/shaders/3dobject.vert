#version 120

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec2 texcoord;
varying vec3 normal;

in vec4 vertex;

void main() {
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	gl_TexCoord[0] = gl_MultiTexCoord0;
	texcoord = gl_MultiTexCoord0.st;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	normal = normalize(gl_NormalMatrix * gl_Normal);
}
