#version 330 core

//DEFINES

in vData {
	vec3 lightDir;
	vec2 texCoord;
	vec3 normal;
	vec4 color;
} fragIn;

out vec4 fragColor;

layout (std140) uniform shaderMatrices {
	mat4 projMatrix;
	mat4 mvMatrix;
	mat4 normalMatrix;
	mat4 colorMatrix;
};

layout (std140) uniform lyricColors {
	vec4 origFill;
	vec4 origStroke;
	vec4 newFill;
	vec4 newStroke;
};

const vec4 epsilon = vec4(1.96e-3);

#ifdef ENABLE_TEXTURING
uniform sampler2D tex;
#define TEXFUNC texture(tex, fragIn.texCoord)
#else
#define TEXFUNC vec4(1.0)
#endif

#ifdef ENABLE_SPECULAR_MAP
uniform sampler2D specularTex;
#endif

#ifdef ENABLE_EMISSION_MAP
uniform sampler2D emissionTex;
#endif

void main() {
	mat4 colorMatrixInternal = colorMatrix;
	vec4 frag = TEXFUNC;

#ifdef ENABLE_VERTEX_COLOR
	frag *= fragIn.color;
#endif

#ifdef ENABLE_LIGHTING
	vec3 n = normalize(fragIn.normal);
	vec3 l = normalize(fragIn.lightDir);

	// Diffuse
	float diff = max(dot(n, l), 0.0);
	float power = 1.0 - 0.02 * length(fragIn.lightDir);
	frag = vec4(frag.rgb * power * diff, frag.a);
#endif
	if (all(lessThanEqual(abs(frag - origFill),epsilon))) { frag = newFill; }
	else if (all(lessThanEqual(abs(frag - origStroke),epsilon))) { frag = newStroke; }
	else { frag = frag * colorMatrixInternal; }

#ifdef ENABLE_LIGHTING
	// Specular
	vec3 refl = reflect(-l, n);
	float spec = dot(refl, n);
	if (power > 0.0) {
	power *= pow(spec, 100);
	#ifdef ENABLE_SPECULAR_MAP
	power *= texture(specularTex, fragIn.texCoord);
	#endif
	frag.rgb += vec3(power, power, power);
	}

	#ifdef ENABLE_EMISSION_MAP
	frag.xyz += texture(emissionTex, texCoord);
	#endif
#endif

	fragColor = frag;
}

