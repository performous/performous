#version 120

in vec4 vertPos;
in vec4 vertTexCoord;

varying float bogus;

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec4 texCoord;


void main() {
	bogus = 0.0;
	// Supply color matrix for fragment shader
	colorMat = colorMatrix;
	texCoord = vertTexCoord;
	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
}

