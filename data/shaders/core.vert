#version 120

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec2 texcoord;

in vec4 vertex;

void main() {
	// Supply color matrix for fragment shader
	colorMat = colorMatrix;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	texcoord = gl_MultiTexCoord0.st;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}

