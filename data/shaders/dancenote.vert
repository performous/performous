#version 120

in vec4 vertPos;
in vec4 vertTexCoord;

varying float bogus;

uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;
uniform vec2 position;

uniform mat4 colorMatrix;
varying mat4 colorMat;
varying vec4 texCoord;

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
	bogus = 0.0;
	
	colorMat = colorMatrix;  // Supply color matrix for fragment shader
	texCoord = vertTexCoord;
	
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

	gl_Position = gl_ModelViewProjectionMatrix * trans * vertPos;
	gl_Position += gl_ModelViewProjectionMatrix * vec4(position.xy, 0, 0);
}

