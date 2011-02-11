#version 120

uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;
uniform vec2 position;

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec2 texcoord;

in vec4 vertex;

mat4 scaleMat(in float sc) {
	return mat4(sc,  0,  0,  0,
	             0, sc,  0,  0,
	             0,  0, sc,  0,
	             0,  0,  0,  1);
}

mat4 rotMat(in float ang) {
	float ca = cos(ang);
	float sa = sin(ang);
	return mat4(ca, -sa, 0, 0,
	            sa,  ca, 0, 0,
	             0,   0, 1, 0,
	             0,   0, 0, 1);
}


void main() {
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	gl_TexCoord[0] = gl_MultiTexCoord0;
	texcoord = gl_MultiTexCoord0.st;
	
	mat4 trans = scaleMat(scale);

	// Cursor arrows
	if (noteType == 0) {
		trans *= scaleMat(1.0 - hitAnim * .5);

	// Regular arrows
	} else if (noteType == 1) {
		trans *= scaleMat(1.0 + hitAnim);

	// Holds
	} else if (noteType == 2) {
		trans *= scaleMat(1.0 + hitAnim);

	// Mines
	} else if (noteType == 3) {
		trans *= scaleMat(1.0 + hitAnim);
		float r = radians(mod(clock*360.0, 360.0)); // They rotate!
		trans *= rotMat(r);
	}

	gl_Position = gl_ModelViewProjectionMatrix * trans * vertex;
	gl_Position += gl_ModelViewProjectionMatrix * vec4(position.xy, 0, 0);
}

