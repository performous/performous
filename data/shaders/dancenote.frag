#version 120

in float bogus;  // Nvidia will overwrite the first in variable with bogus data, so as a workaround we put a bogus variable here
in mat4 colorMat;

uniform sampler2D tex;
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

	texel = texture2D(tex, gl_TexCoord[0].st).rgba;

	// Regular arrows or holds
	if (noteType == 1 || noteType == 2) colorGlow(texel);

	gl_FragColor = vec4(0,0,0,bogus * 1e-10) + colorMat * texel;
}

