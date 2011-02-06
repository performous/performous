uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;
uniform vec2 position;

#define deg2rad 0.0174532925

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


void main()
{
	gl_FrontColor = gl_Color;
	gl_BackColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;

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
		float r = float(mod(int(clock*360.0), 360)) * deg2rad; // They rotate!
		trans *= rotMat(r);
	}

	gl_Position = gl_ModelViewProjectionMatrix * trans * gl_Vertex;
	gl_Position += gl_ModelViewProjectionMatrix * vec4(position.xy, 0, 0);
}

