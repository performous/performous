#extension GL_ARB_texture_rectangle : enable

uniform int texMode;
uniform sampler2D tex;
uniform sampler2DRect texRect;

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

	gl_FragColor = texel * gl_Color;
}
