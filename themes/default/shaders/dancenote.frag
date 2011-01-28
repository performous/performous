#extension GL_ARB_texture_rectangle : enable

uniform int texMode;
uniform sampler2D tex;
uniform sampler2DRect texRect;
uniform int noteType;
uniform float hitAnim;
uniform float clock;
uniform float scale;


void colorGlow(inout vec4 c) {
	c = vec4(
		min(c.r + hitAnim *.5, 1.0),
		min(c.g + hitAnim *.5, 1.0),
		min(c.b + hitAnim *.5, 1.0),
		max(c.a - hitAnim, 0.0));
}


void main()
{
	vec4 texel;

	if (texMode == 1) {
		texel = texture2D(tex, gl_TexCoord[0].st).rgba;
	} else if (texMode == 2) {
		texel = texture2DRect(texRect, gl_TexCoord[0].st).rgba;
	} else if (texMode == 0) {
		texel = vec4(1.0, 1.0, 1.0, 1.0);
	} else {
		texel = vec4(1.0, 0.0, 1.0, 1.0); // Magenta to highlight
	}

	// Cursor arrows
	if (noteType == 0) {
		/* no op */

	// Regular arrows
	} else if (noteType == 1) {
		colorGlow(texel);

	// Holds
	} else if (noteType == 2) {
		colorGlow(texel);

	// Mines
	} else if (noteType == 3) {
		/* no op */

	}

	gl_FragColor = texel;
}

