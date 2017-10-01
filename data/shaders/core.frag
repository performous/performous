#version 330

//DEFINES

uniform mat4 colorMatrix;

in vec3 vNormal;
in vec3 vLightDir;
in vec4 vColor;

#ifdef ENABLE_BOGUS
in float bogus;  // Workaround for http://www.nvnews.net/vbulletin/showthread.php?p=2401097
#endif

out vec3 outNormal;
out vec4 outColor;

#ifdef ENABLE_LIGHTING
out vec3 outLightDir;
#endif

#if defined(ENABLE_TEXTURING) || defined(ENABLE_SPECULAR_MAP) || defined(ENABLE_EMISSION_MAP)
out vec2 texCoord;
#endif

#ifdef ENABLE_TEXTURING
#if ENABLE_TEXTURING == 1
uniform sampler2DRect tex;
#elif ENABLE_TEXTURING == 2
uniform sampler2D tex;
#else
#error Unknown texturing mode in ENABLE_TEXTURING
#endif
#define TEXFUNC texture(tex, texCoord)
#endif

#ifndef TEXFUNC
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

#ifdef ENABLE_BOGUS
	frag.a += 1e-14 * bogus;  // Convince the compiler not to optimize away the bogus variable
#endif

#ifdef ENABLE_VERTEX_COLOR
	frag *= vColor;
#endif

#ifdef ENABLE_LIGHTING
	vec3 n = normalize(vNormal);
	vec3 l = normalize(vLightDir);

	// Diffuse
	float diff = max(dot(n, l), 0.0);
	float power = 1.0 - 0.02 * length(outLightDir);
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

	outColor = frag;
}

