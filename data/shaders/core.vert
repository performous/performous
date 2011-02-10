#version 120

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec2 texcoord;

in vec4 vertex;

void main() {
	// Supply color matrix for fragment shader
	colorMat = colorMatrix;
	for (int i = 0; i < 4; ++i) {
		float c = gl_Color[i];
		for (int j = 0; j < 4; ++j) colorMat[j][i] *= c;
	}
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	texcoord = gl_MultiTexCoord0.st;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}

