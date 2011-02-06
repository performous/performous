#extension GL_ARB_texture_rectangle : enable

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

	gl_FragColor = texel;
}

