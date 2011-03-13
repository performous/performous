#version 120
#extension GL_ARB_texture_rectangle : require

//DEFINES

uniform mat4 colorMatrix;

#ifdef ENABLE_BOGUS
in float bogus;  // Workaround for http://www.nvnews.net/vbulletin/showthread.php?p=2401097
#endif

in vec3 normal;
in vec4 color;

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

#ifdef ENABLE_BOGUS
	frag.a += 1e-14 * bogus;  // Convince the compiler not to optimize away the bogus variable
#endif

#ifdef ENABLE_VERTEX_COLOR
	frag *= color;
#endif

#ifdef ENABLE_LIGHTING
	vec3 lightDir = normalize(vec3(-50.0, 5.0, -15.0));
	const vec3 ambient = vec3(0.1, 0.1, 0.1);

	float NdotL = max(dot(normalize(normal), lightDir), 0.0);
	frag = vec4(ambient + frag.rgb * NdotL, frag.a);
#endif

	gl_FragColor = colorMatrix * frag;
}

