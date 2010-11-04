#extension GL_ARB_texture_rectangle : enable

uniform int texMode;
uniform sampler2D tex;
uniform sampler2DRect texRect;

uniform float anim;

void main()
{
	vec4 texel;

	if (texMode == 1) {
		texel = texture2D(tex, gl_TexCoord[0].st).rgba;
	} else if (texMode == 2) {
		texel = texture2DRect(texRect, gl_TexCoord[0].st).rgba;
	} else if (texMode == 0) {
		texel = gl_Color;
	} else {
		texel = vec4(1.0, 0.0, 1.0, 1.0); // Magenta to highlight
	}

	float r = anim * 0.5;
	float g = anim * 0.5 + 0.5;
	float b = anim * 0.3 + 0.7;

	gl_FragColor = vec4(texel.r * r, texel.g * g, texel.b * b, texel.a);
}
