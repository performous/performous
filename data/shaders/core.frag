#version 120
#extension GL_ARB_texture_rectangle : require

//DEFINES

in float bogus;  // Nvidia will overwrite the first in variable with bogus data, so as a workaround we put a bogus variable here
in mat4 colorMat;
in vec4 texCoord;
in vec3 normal;
in vec4 color;

#ifdef SURFACE
uniform sampler2DRect tex;
#define TEXFUNC texture2DRect(tex, texCoord.st)
#endif

#ifdef TEXTURE
uniform sampler2D tex;
#define TEXFUNC texture2D(tex, texCoord.st)
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

	gl_FragColor = vec4(0,0,0,bogus * 1e-10) + colorMat * frag;
}

