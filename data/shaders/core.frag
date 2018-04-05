#version 330 core

out vec4 fragColor;

//DEFINES

uniform mat4 colorMatrix;

in vec3 normal;
in vec4 color;

#ifdef ENABLE_LIGHTING
in vec3 lightDir;
#endif

#if defined(ENABLE_TEXTURING) || defined(ENABLE_SPECULAR_MAP) || defined(ENABLE_EMISSION_MAP)
in vec2 texCoord;
#endif

#ifdef ENABLE_TEXTURING
uniform sampler2D tex;
#define TEXFUNC texture(tex, texCoord)
#else
#define TEXFUNC vec4(1,1,1,1)
#endif

#ifdef ENABLE_SPECULAR_MAP
uniform sampler2D specularTex;
#endif

#ifdef ENABLE_EMISSION_MAP
uniform sampler2D emissionTex;
#endif

void main() {
	vec4 frag = TEXFUNC;

#ifdef ENABLE_VERTEX_COLOR
	frag *= color;
#endif

#ifdef ENABLE_LIGHTING
	vec3 n = normalize(normal);
	vec3 l = normalize(lightDir);

	// Diffuse
	float diff = max(dot(n, l), 0.0);
	float power = 1.0 - 0.02 * length(lightDir);
	frag = vec4(frag.rgb * power * diff, frag.a);
#endif

	frag = colorMatrix * frag;  // Colorize

#ifdef ENABLE_LIGHTING
	// Specular
	vec3 refl = reflect(-l, n);
	float spec = dot(refl, n);
	if (power > 0.0) {
		power *= pow(spec, 100);
		#ifdef ENABLE_SPECULAR_MAP
		power *= texture(specularTex, texCoord);
		#endif
		frag.rgb += vec3(power, power, power);
	}

	#ifdef ENABLE_EMISSION_MAP
	frag.xyz += texture(emissionTex, texCoord);
	#endif
#endif

	fragColor = frag;
}

