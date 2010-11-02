uniform int texMode;
uniform sampler2D tex;
uniform sampler2DRect texRect;

varying vec3 normal;

void main()
{
	const vec3 lightDir = normalize(vec3(-50.0, 5.0, -15.0));
	const vec3 ambient = vec3(0.1, 0.1, 0.1);
	vec4 texel;

	if (texMode == 1) {
		texel = texture2D(tex, gl_TexCoord[0].st).rgba;
	} else if (texMode == 2) {
		texel = texture2DRect(texRect, gl_TexCoord[0].st).rgba;
	} else if (texMode == 0) {
		texel = vec4(1.0, 1.0, 1.0, 1.0);
	} else {
		texel = vec4(1.0, 0.0, 0.0, 1.0);
	}

	float NdotL = max(dot(normalize(normal), lightDir), 0.0);
	vec3 color = ambient + texel.rgb * gl_Color.rgb * NdotL;

	gl_FragColor = vec4(color, texel.a * gl_Color.a);
}
