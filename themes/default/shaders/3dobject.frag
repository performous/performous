uniform int texMode;
uniform sampler2D tex;
uniform sampler2DRect texrect;

void main()
{
	vec4 texel;
	vec4 color;
	if (texMode == 1) {
		texel = texture2D(tex, gl_TexCoord[0].st).rgba;
	} else if (texMode == 2) {
		texel = texture2DRect(texrect, gl_TexCoord[0].st).rgba;
	} else if (texMode == 0) {
		texel = gl_Color;
	} else {
		texel = vec4(1.0,0.0,0.0,1.0);
	}
	gl_FragColor = vec4(texel.rgb*gl_Color.rgb, texel.a*gl_Color.a);
}
