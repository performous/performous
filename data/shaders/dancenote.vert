#version 120

// Input from glVertexAttribPointer
/* layout (location = 0) */ in vec4 vertPos;
/* layout (location = 1) */ in vec4 vertTexCoord;
/* layout (location = 2) */ in vec3 vertNormal;
/* layout (location = 3) */ in vec4 vertColor;

varying float bogus;

uniform mat4 colorMatrix;
varying mat4 colorMat;

// Per-vextex for fragment shader (if no geometry shader)
varying vec4 texCoord;
varying vec3 normal;
varying vec4 color;

// Per-vertex for geometry shader (if one exists)
varying vec4 vTexCoord;
varying vec3 vNormal;
varying vec4 vColor;


uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;
uniform vec2 position;

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
	colorMat = colorMatrix;  // In case no geometry shader is used (otherwise it sets this)
	vTexCoord = texCoord = vertTexCoord;
	vNormal = normal = normalize(gl_NormalMatrix * vertNormal);
	vColor = color = vertColor;
	
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

	// Glow color for regular arrows or holds
	if (noteType == 1 || noteType == 2) {
		vColor = color = vec4(
		  min(color.r + hitAnim *.5, 1.0),
		  min(color.g + hitAnim *.5, 1.0),
		  min(color.b + hitAnim *.5, 1.0),
		  max(color.a - hitAnim, 0.0)
		);
	}

	gl_Position = gl_ModelViewProjectionMatrix * trans * vertPos;
	gl_Position += gl_ModelViewProjectionMatrix * vec4(position.xy, 0, 0);
}

