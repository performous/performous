#version 120
#extension GL_ARB_texture_rectangle : require

//DEFINES

#ifdef ENABLE_BOGUS
in float bogus;  // Nvidia will overwrite the first in variable with bogus data, so as a workaround we put a bogus variable here
#endif

in mat4 colorMat;

#ifdef ENABLE_LIGHTING
in vec3 normal;
#endif

#ifdef ENABLE_VERTEX_COLOR
in vec4 color;
#endif

#ifdef SURFACE
#endif

#ifdef ENABLE_TEXTURING

in vec4 texCoord;

#if ENABLE_TEXTURING == 1

uniform sampler2DRect tex;
#define TEXFUNC texture2DRect(tex, texCoord.st)

#elif ENABLE_TEXTURING == 2

uniform sampler2D tex;
#define TEXFUNC texture2D(tex, texCoord.st)

#else
#error Unknown texturing mode in ENABLE_TEXTURING
#endif
#endif

#ifndef TEXFUNC
#define TEXFUNC vec4(1,1,1,1)
#endif

void main() {
	vec4 frag = TEXFUNC;

#ifdef ENABLE_LIGHTING
	vec3 lightDir = normalize(vec3(-50.0, 5.0, -15.0));
	const vec3 ambient = vec3(0.1, 0.1, 0.1);

	float NdotL = max(dot(normalize(normal), lightDir), 0.0);
	frag = vec4(ambient + frag.rgb * NdotL, frag.a);
#endif
#ifdef ENABLE_VERTEX_COLOR
	frag *= color;
#endif
	gl_FragColor = colorMat * frag;
#ifdef ENABLE_BOGUS
	gl_FragColor += vec4(0,0,0,bogus * 1e-10)
#endif
}

